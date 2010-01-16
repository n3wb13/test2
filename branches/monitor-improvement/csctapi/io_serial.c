   /*
    io_serial.c
    Serial port input/output functions

    This file is part of the Unix driver for Towitoko smartcard readers
    Copyright (C) 2000 2001 Carlos Prados <cprados@yahoo.com>

    This version is modified by doz21 to work in a special manner ;)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "defines.h"
#include "../globals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef OS_HPUX
#include <sys/modem.h>
#endif
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_POLL
#include <sys/poll.h>
#else
#include <sys/signal.h>
#include <sys/types.h>
#endif
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include "io_serial.h"
#include "mc_global.h"

#ifdef OS_LINUX
#include <linux/serial.h>
#endif

#define IO_SERIAL_FILENAME_LENGTH 	32

/*
 * Internal functions declaration
 */

static int IO_Serial_Bitrate(int bitrate);

static bool IO_Serial_WaitToRead (unsigned delay_ms, unsigned timeout_ms);

static bool IO_Serial_WaitToWrite (unsigned delay_ms, unsigned timeout_ms);

static bool IO_Serial_InitPnP (IO_Serial * io);

static void IO_Serial_Clear (IO_Serial * io);

static int _in_echo_read = 0;
int io_serial_need_dummy_char = 0;

extern int fdmc;

#if defined(TUXBOX) && defined(PPC)
void IO_Serial_Ioctl_Lock(int flag)
{
  extern int *oscam_sem;
  if ((reader[ridx].typ != R_DB2COM1) && (reader[ridx].typ != R_DB2COM2)) return;
  if (!flag)
    *oscam_sem=0;
  else while (*oscam_sem!=reader[ridx].typ)
  {
    while (*oscam_sem)
    usleep((reader[ridx].typ)*2000); //FIXME is this right ?!?!
    *oscam_sem=reader[ridx].typ;
    usleep(1000);
  }
}

static bool IO_Serial_DTR_RTS_dbox2(int mcport, int dtr, int set)
{
  int rc;
  unsigned short msr;
  unsigned int mbit;
  unsigned short rts_bits[2]={ 0x10, 0x800};
  unsigned short dtr_bits[2]={0x100,     0};

#ifdef DEBUG_IO
printf("IO: multicam.o %s %s\n", dtr ? "dtr" : "rts", set ? "set" : "clear"); fflush(stdout);
#endif
  if ((rc=ioctl(fdmc, GET_PCDAT, &msr))>=0)
  {
    if (dtr)		// DTR
    {
      if (dtr_bits[mcport])
      {
        if (set)
          msr&=(unsigned short)(~dtr_bits[mcport]);
        else
          msr|=dtr_bits[mcport];
        rc=ioctl(fdmc, SET_PCDAT, &msr);
      }
      else
        rc=0;		// Dummy, can't handle using multicam.o
    }
    else		// RTS
    {
      if (set)
        msr&=(unsigned short)(~rts_bits[mcport]);
      else
        msr|=rts_bits[mcport];
      rc=ioctl(fdmc, SET_PCDAT, &msr);
    }
  }
  return((rc<0) ? FALSE : TRUE);
}
#endif

bool IO_Serial_DTR_RTS(int dtr, int set)
{
	unsigned int msr;
	unsigned int mbit;

#if defined(TUXBOX) && defined(PPC)
	if ((reader[ridx].typ == R_DB2COM1) || (reader[ridx].typ == R_DB2COM2))
		return(IO_Serial_DTR_RTS_dbox2(reader[ridx].typ == R_DB2COM2, dtr, set));
#endif

	mbit=(dtr) ? TIOCM_DTR : TIOCM_RTS;
#if defined(TIOCMBIS) && defined(TIOBMBIC)
	if (ioctl (reader[ridx].handle, set ? TIOCMBIS : TIOCMBIC, &mbit) < 0)
		return FALSE;
#else
	if (ioctl(reader[ridx].handle, TIOCMGET, &msr) < 0)
		return FALSE;
	if (set)
		msr|=mbit;
	else
		msr&=~mbit;
	return((ioctl(reader[ridx].handle, TIOCMSET, &msr)<0) ? FALSE : TRUE);
#endif
}

/*
 * Public functions definition
 */

IO_Serial * IO_Serial_New (int mhz, int cardmhz)
{
	IO_Serial *io;

	io = (IO_Serial *) malloc (sizeof (IO_Serial));

	if (io != NULL)
		IO_Serial_Clear (io);

	return io;
}

bool IO_Serial_Init (IO_Serial * io, int reader_type)
{
	reader[ridx].typ = reader_type;
	if (reader[ridx].typ != R_INTERNAL)
		IO_Serial_InitPnP (io);

	if(reader[ridx].typ!=R_INTERNAL)
		IO_Serial_Flush();

	return TRUE;
}

bool IO_Serial_GetPropertiesOld (IO_Serial * io)
{
	struct termios currtio;
	speed_t i_speed, o_speed;
	unsigned int mctl;

#ifdef SCI_DEV
	if(reader[ridx].typ == R_INTERNAL)
		return FALSE;
#endif

	if (io->input_bitrate != 0 && io->output_bitrate != 0) //properties are already filled
	  return TRUE;

	if (tcgetattr (reader[ridx].handle, &currtio) != 0)
		return FALSE;

	o_speed = cfgetospeed (&currtio);

	switch (o_speed)
	{
#ifdef B0
		case B0:
			io->output_bitrate = 0;
			break;
#endif
#ifdef B50
		case B50:
			io->output_bitrate = 50;
			break;
#endif
#ifdef B75
		case B75:
			io->output_bitrate = 75;
			break;
#endif
#ifdef B110
		case B110:
			io->output_bitrate = 110;
			break;
#endif
#ifdef B134
		case B134:
			io->output_bitrate = 134;
			break;
#endif
#ifdef B150
		case B150:
			io->output_bitrate = 150;
			break;
#endif
#ifdef B200
		case B200:
			io->output_bitrate = 200;
			break;
#endif
#ifdef B300
		case B300:
			io->output_bitrate = 300;
			break;
#endif
#ifdef B600
		case B600:
			io->output_bitrate = 600;
			break;
#endif
#ifdef B1200
		case B1200:
			io->output_bitrate = 1200;
			break;
#endif
#ifdef B1800
		case B1800:
			io->output_bitrate = 1800;
			break;
#endif
#ifdef B2400
		case B2400:
			io->output_bitrate = 2400;
			break;
#endif
#ifdef B4800
		case B4800:
			io->output_bitrate = 4800;
			break;
#endif
#ifdef B9600
		case B9600:
			io->output_bitrate = 9600;
			break;
#endif
#ifdef B19200
		case B19200:
			io->output_bitrate = 19200;
			break;
#endif
#ifdef B38400
		case B38400:
			io->output_bitrate = 38400;
			break;
#endif
#ifdef B57600
		case B57600:
			io->output_bitrate = 57600;
			break;
#endif
#ifdef B115200
		case B115200:
			io->output_bitrate = 115200;
			break;
#endif
#ifdef B230400
		case B230400:
			io->output_bitrate = 230400;
			break;
#endif
		default:
			io->output_bitrate = 1200;
			break;
	}

	i_speed = cfgetispeed (&currtio);

	switch (i_speed)
	{
#ifdef B0
		case B0:
			io->input_bitrate = 0;
			break;
#endif
#ifdef B50
		case B50:
			io->input_bitrate = 50;
			break;
#endif
#ifdef B75
		case B75:
			io->input_bitrate = 75;
			break;
#endif
#ifdef B110
		case B110:
			io->input_bitrate = 110;
			break;
#endif
#ifdef B134
		case B134:
			io->input_bitrate = 134;
			break;
#endif
#ifdef B150
		case B150:
			io->input_bitrate = 150;
			break;
#endif
#ifdef B200
		case B200:
			io->input_bitrate = 200;
			break;
#endif
#ifdef B300
		case B300:
			io->input_bitrate = 300;
			break;
#endif
#ifdef B600
		case B600:
			io->input_bitrate = 600;
			break;
#endif
#ifdef B1200
		case B1200:
			io->input_bitrate = 1200;
			break;
#endif
#ifdef B1800
		case B1800:
			io->input_bitrate = 1800;
			break;
#endif
#ifdef B2400
		case B2400:
			io->input_bitrate = 2400;
			break;
#endif
#ifdef B4800
		case B4800:
			io->input_bitrate = 4800;
			break;
#endif
#ifdef B9600
		case B9600:
			io->input_bitrate = 9600;
			break;
#endif
#ifdef B19200
		case B19200:
			io->input_bitrate = 19200;
			break;
#endif
#ifdef B38400
		case B38400:
			io->input_bitrate = 38400;
			break;
#endif
#ifdef B57600
		case B57600:
			io->input_bitrate = 57600;
			break;
#endif
#ifdef B115200
		case B115200:
			io->input_bitrate = 115200;
			break;
#endif
#ifdef B230400
		case B230400:
			io->input_bitrate = 230400;
			break;
#endif
		default:
			io->input_bitrate = 1200;
			break;
	}

	switch (currtio.c_cflag & CSIZE)
	{
		case CS5:
			io->bits = 5;
			break;
		case CS6:
			io->bits = 6;
			break;
		case CS7:
			io->bits = 7;
			break;
		case CS8:
			io->bits = 8;
			break;
	}

	if (((currtio.c_cflag) & PARENB) == PARENB)
	{
		if (((currtio.c_cflag) & PARODD) == PARODD)
			io->parity = PARITY_ODD;
		else
			io->parity = PARITY_EVEN;
	}
	else
	{
		io->parity = PARITY_NONE;
	}

	if (((currtio.c_cflag) & CSTOPB) == CSTOPB)
		io->stopbits = 2;
	else
		io->stopbits = 1;

	if (ioctl (reader[ridx].handle, TIOCMGET, &mctl) < 0)
		return FALSE;

	io->dtr = ((mctl & TIOCM_DTR) ? IO_SERIAL_HIGH : IO_SERIAL_LOW);
	io->rts = ((mctl & TIOCM_RTS) ? IO_SERIAL_HIGH : IO_SERIAL_LOW);

#ifdef DEBUG_IO
	printf("IO: Getting properties: %ld bps; %d bits/byte; %s parity; %d stopbits; dtr=%d; rts=%d\n", io->input_bitrate, io->bits, io->parity == PARITY_EVEN ? "Even" : io->parity == PARITY_ODD ? "Odd" : "None", io->stopbits, io->dtr, io->rts);
#endif

	return TRUE;
}

bool IO_Serial_SetPropertiesOld (IO_Serial * io)
{
   struct termios newtio;

#ifdef SCI_DEV
   if(reader[ridx].typ == R_INTERNAL)
      return FALSE;
#endif

   //	printf("IO: Setting properties: reader_type%d, %ld bps; %d bits/byte; %s parity; %d stopbits; dtr=%d; rts=%d\n", reader[ridx].typ, io->input_bitrate, io->bits, io->parity == PARITY_EVEN ? "Even" : io->parity == PARITY_ODD ? "Odd" : "None", io->stopbits, io->dtr, io->rts);
   memset (&newtio, 0, sizeof (newtio));


   /* Set the bitrate */
#ifdef OS_LINUX
   if (reader[ridx].mhz == reader[ridx].cardmhz)
#endif
   { //no overclocking
     cfsetospeed(&newtio, IO_Serial_Bitrate(io->output_bitrate));
     cfsetispeed(&newtio, IO_Serial_Bitrate(io->input_bitrate));
     cs_debug("standard baudrate: cardmhz=%d mhz=%d -> effective baudrate %lu", reader[ridx].cardmhz, reader[ridx].mhz, io->output_bitrate);
   }
#ifdef OS_LINUX
   else { //over or underclocking
    /* these structures are only available on linux as fas as we know so limit this code to OS_LINUX */
    struct serial_struct nuts;
    ioctl(reader[ridx].handle, TIOCGSERIAL, &nuts);
    int custom_baud = io->output_bitrate * reader[ridx].mhz / reader[ridx].cardmhz;
    nuts.custom_divisor = (nuts.baud_base + (custom_baud/2))/ custom_baud;
    cs_debug("custom baudrate: cardmhz=%d mhz=%d custom_baud=%d baud_base=%d divisor=%d -> effective baudrate %d",
	                      reader[ridx].cardmhz, reader[ridx].mhz, custom_baud, nuts.baud_base, nuts.custom_divisor, nuts.baud_base/nuts.custom_divisor);
    nuts.flags &= ~ASYNC_SPD_MASK;
    nuts.flags |= ASYNC_SPD_CUST;
    ioctl(reader[ridx].handle, TIOCSSERIAL, &nuts);
    cfsetospeed(&newtio, IO_Serial_Bitrate(38400));
    cfsetispeed(&newtio, IO_Serial_Bitrate(38400));
   }
#endif

   /* Set the character size */
   switch (io->bits)
   {
		case 5:
			newtio.c_cflag |= CS5;
			break;

		case 6:
			newtio.c_cflag |= CS6;
			break;

		case 7:
			newtio.c_cflag |= CS7;
			break;

		case 8:
			newtio.c_cflag |= CS8;
			break;
	}

	/* Set the parity */
	switch (io->parity)
	{
		case PARITY_ODD:
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			break;

		case PARITY_EVEN:
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;

		case PARITY_NONE:
			newtio.c_cflag &= ~PARENB;
			break;
	}

	/* Set the number of stop bits */
	switch (io->stopbits)
	{
		case 1:
			newtio.c_cflag &= (~CSTOPB);
			break;
		case 2:
			newtio.c_cflag |= CSTOPB;
			break;
	}

	/* Selects raw (non-canonical) input and output */
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_oflag &= ~OPOST;
#if 1
	newtio.c_iflag |= IGNPAR;
	/* Ignore parity errors!!! Windows driver does so why shouldn't I? */
#endif
	/* Enable receiber, hang on close, ignore control line */
	newtio.c_cflag |= CREAD | HUPCL | CLOCAL;

	/* Read 1 byte minimun, no timeout specified */
	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 0;

//	tcdrain(reader[ridx].handle);
	if (tcsetattr (reader[ridx].handle, TCSANOW, &newtio) < 0)
		return FALSE;
//	tcflush(reader[ridx].handle, TCIOFLUSH);
//	if (tcsetattr (reader[ridx].handle, TCSAFLUSH, &newtio) < 0)
//		return FALSE;

	IO_Serial_Ioctl_Lock(1);
	IO_Serial_DTR_RTS(0, io->rts == IO_SERIAL_HIGH);
	IO_Serial_DTR_RTS(1, io->dtr == IO_SERIAL_HIGH);
	IO_Serial_Ioctl_Lock(0);

#ifdef DEBUG_IO
	printf("IO: Setting properties: reader_type%d, %ld bps; %d bits/byte; %s parity; %d stopbits; dtr=%d; rts=%d\n", reader[ridx].typ, io->input_bitrate, io->bits, io->parity == PARITY_EVEN ? "Even" : io->parity == PARITY_ODD ? "Odd" : "None", io->stopbits, io->dtr, io->rts);
#endif
	return TRUE;
}

bool IO_Serial_SetProperties (struct termios newtio)
{
   if(reader[ridx].typ == R_INTERNAL)
      return FALSE;

	if (tcsetattr (reader[ridx].handle, TCSANOW, &newtio) < 0)
		return FALSE;
//	tcflush(reader[ridx].handle, TCIOFLUSH);
//	if (tcsetattr (reader[ridx].handle, TCSAFLUSH, &newtio) < 0)
//		return FALSE;

	unsigned int mctl;
	if (ioctl (reader[ridx].handle, TIOCMGET, &mctl) < 0)
		return FALSE;

	int dtr = ((mctl & TIOCM_DTR) ? IO_SERIAL_HIGH : IO_SERIAL_LOW);
	int rts = ((mctl & TIOCM_RTS) ? IO_SERIAL_HIGH : IO_SERIAL_LOW);

	IO_Serial_Ioctl_Lock(1);
	IO_Serial_DTR_RTS(0, rts == IO_SERIAL_HIGH);
	IO_Serial_DTR_RTS(1, dtr == IO_SERIAL_HIGH);
	IO_Serial_Ioctl_Lock(0);

#ifdef DEBUG_IO
	printf("IO: Setting properties\n");
#endif

	return TRUE;
}

int IO_Serial_SetParity (BYTE parity)
{
	if(reader[ridx].typ == R_INTERNAL)
		return TRUE;

	if ((parity != PARITY_EVEN) && (parity != PARITY_ODD) && (parity != PARITY_NONE))
		return FALSE;

	struct termios tio;
	int current_parity;
	// Get current parity
	if (tcgetattr (reader[ridx].handle, &tio) != 0)
	  return FALSE;

	if (((tio.c_cflag) & PARENB) == PARENB)
	{
		if (((tio.c_cflag) & PARODD) == PARODD)
			current_parity = PARITY_ODD;
		else
			current_parity = PARITY_EVEN;
	}
	else
	{
		current_parity = PARITY_NONE;
	}

#ifdef DEBUG_IFD
	printf ("IFD: Setting parity from %s to %s\n",
	current_parity == PARITY_ODD ? "Odd" :
	current_parity == PARITY_NONE ? "None" :
	current_parity == PARITY_EVEN ? "Even" : "Invalid",
	parity == PARITY_ODD ? "Odd" :
	parity == PARITY_NONE ? "None" :
	parity == PARITY_EVEN ? "Even" : "Invalid");
#endif

	if (current_parity != parity)
	{

		// Set the parity
		switch (parity)
		{
			case PARITY_ODD:
				tio.c_cflag |= PARENB;
				tio.c_cflag |= PARODD;
				break;

			case PARITY_EVEN:
				tio.c_cflag |= PARENB;
				tio.c_cflag &= ~PARODD;
				break;

			case PARITY_NONE:
				tio.c_cflag &= ~PARENB;
				break;
		}
		if (!IO_Serial_SetProperties (tio))
			return FALSE;
	}

	return TRUE;
}

void IO_Serial_Flush ()
{
	BYTE b;
	while(IO_Serial_Read(1000, 1, &b));
}


void IO_Serial_GetPnPId (IO_Serial * io, BYTE * pnp_id, unsigned *length)
{
	(*length) = io->PnP_id_size;
	memcpy (pnp_id, io->PnP_id, io->PnP_id_size);
}

bool IO_Serial_Read (unsigned timeout, unsigned size, BYTE * data)
{
	BYTE c;
	uint count = 0;
#ifdef SH4
	bool readed;
	struct timeval tv, tv_spent;
#endif

	if((reader[ridx].typ != R_INTERNAL) && (wr>0))
	{
		BYTE buf[256];
		int n = wr;
		wr = 0;

		if(!IO_Serial_Read (timeout, n, buf))
		{
			return FALSE;
		}
	}

#ifdef DEBUG_IO
	printf ("IO: Receiving: ");
	fflush (stdout);
#endif
	for (count = 0; count < size * (_in_echo_read ? (1+io_serial_need_dummy_char) : 1); count++)
	{
#ifdef SH4
		gettimeofday(&tv,0);
		memcpy(&tv_spent,&tv,sizeof(struct timeval));
		readed=FALSE;
		while( (((tv_spent.tv_sec-tv.tv_sec)*1000) + ((tv_spent.tv_usec-tv.tv_usec)/1000L))<timeout )
 		{
 			if (read (reader[ridx].handle, &c, 1) == 1)
 			{
 				readed=TRUE;
				break;
 			}
 			gettimeofday(&tv_spent,0);
		}
		if(!readed) return FALSE;

		data[_in_echo_read ? count/(1+io_serial_need_dummy_char) : count] = c;
#ifdef DEBUG_IO
		printf ("%X ", c);
		fflush (stdout);
#endif
#else
		if (IO_Serial_WaitToRead (0, timeout))
		{
			if (read (reader[ridx].handle, &c, 1) != 1)
			{
#ifdef DEBUG_IO
				printf ("ERROR\n");
				fflush (stdout);
#endif
				return FALSE;
			}
			data[_in_echo_read ? count/(1+io_serial_need_dummy_char) : count] = c;

#ifdef DEBUG_IO
			printf ("%X ", c);
			fflush (stdout);
#endif
		}
		else
		{
#ifdef DEBUG_IO
			printf ("TIMEOUT\n");
			fflush (stdout);
#endif
			tcflush (reader[ridx].handle, TCIFLUSH);
			return FALSE;
		}
#endif
	}

    _in_echo_read = 0;

#ifdef DEBUG_IO
	printf ("\n");
	fflush (stdout);
#endif

	return TRUE;
}




bool IO_Serial_Write (unsigned delay, unsigned size, BYTE * data)
{
	unsigned count, to_send, i_w;
    BYTE data_w[512];
#ifdef DEBUG_IO
	unsigned i;

	printf ("IO: Sending: ");
	fflush (stdout);
#endif
	/* Discard input data from previous commands */
//	tcflush (reader[ridx].handle, TCIFLUSH);

	for (count = 0; count < size; count += to_send)
	{
//		if(reader[ridx].typ == R_INTERNAL)
//			to_send = 1;
//		else
			to_send = (delay? 1: size);

		if (IO_Serial_WaitToWrite (delay, 1000))
		{
            for (i_w=0; i_w < to_send; i_w++) {
            data_w [(1+io_serial_need_dummy_char)*i_w] = data [count + i_w];
            if (io_serial_need_dummy_char) {
              data_w [2*i_w+1] = 0x00;
              }
            }
            unsigned int u = write (reader[ridx].handle, data_w, (1+io_serial_need_dummy_char)*to_send);
            _in_echo_read = 1;
            if (u != (1+io_serial_need_dummy_char)*to_send)
			{
#ifdef DEBUG_IO
				printf ("ERROR\n");
				fflush (stdout);
#endif
				if(reader[ridx].typ != R_INTERNAL)
					wr += u;
				return FALSE;
			}

			if(reader[ridx].typ != R_INTERNAL)
				wr += to_send;

#ifdef DEBUG_IO
			for (i=0; i<(1+io_serial_need_dummy_char)*to_send; i++)
				printf ("%X ", data_w[count + i]);
			fflush (stdout);
#endif
		}
		else
		{
#ifdef DEBUG_IO
			printf ("TIMEOUT\n");
			fflush (stdout);
#endif
//			tcflush (reader[ridx].handle, TCIFLUSH);
			return FALSE;
		}
	}

#ifdef DEBUG_IO
	printf ("\n");
	fflush (stdout);
#endif

	return TRUE;
}

bool IO_Serial_Close (IO_Serial * io)
{

#ifdef DEBUG_IO
	printf ("IO: Clossing serial port %s\n", reader[ridx].device);
#endif

#if defined(TUXBOX) && defined(PPC)
	close(fdmc);
#endif
	if (close (reader[ridx].handle) != 0)
		return FALSE;

	IO_Serial_Clear (io);

	return TRUE;
}

/*
 * Internal functions definition
 */

static int IO_Serial_Bitrate(int bitrate)
{
#ifdef B230400
	if ((bitrate)>=230400) return B230400;
#endif
#ifdef B115200
	if ((bitrate)>=115200) return B115200;
#endif
#ifdef B57600
	if ((bitrate)>=57600) return B57600;
#endif
#ifdef B38400
	if ((bitrate)>=38400) return B38400;
#endif
#ifdef B19200
	if ((bitrate)>=19200) return B19200;
#endif
#ifdef B9600
	if ((bitrate)>=9600) return B9600;
#endif
#ifdef B4800
	if ((bitrate)>=4800) return B4800;
#endif
#ifdef B2400
	if ((bitrate)>=2400) return B2400;
#endif
#ifdef B1800
	if ((bitrate)>=1800) return B1800;
#endif
#ifdef B1200
	if ((bitrate)>=1200) return B1200;
#endif
#ifdef B600
	if ((bitrate)>=600) return B600;
#endif
#ifdef B300
	if ((bitrate)>=300) return B300;
#endif
#ifdef B200
	if ((bitrate)>=200) return B200;
#endif
#ifdef B150
	if ((bitrate)>=150) return B150;
#endif
#ifdef B134
	if ((bitrate)>=134) return B134;
#endif
#ifdef B110
	if ((bitrate)>=110) return B110;
#endif
#ifdef B75
	if ((bitrate)>=75) return B75;
#endif
#ifdef B50
	if ((bitrate)>=50) return B50;
#endif
#ifdef B0
	if ((bitrate)>=0) return B0;
#endif
	return 0;	/* Should never get here */
}

static bool IO_Serial_WaitToRead (unsigned delay_ms, unsigned timeout_ms)
{
   fd_set rfds;
   fd_set erfds;
   struct timeval tv;
   int select_ret;
   int in_fd;

   if (delay_ms > 0)
   {
#ifdef HAVE_NANOSLEEP
      struct timespec req_ts;

      req_ts.tv_sec = delay_ms / 1000;
      req_ts.tv_nsec = (delay_ms % 1000) * 1000000L;
      nanosleep (&req_ts, NULL);
#else
      usleep (delay_ms * 1000L);
#endif
   }

   in_fd=reader[ridx].handle;

   FD_ZERO(&rfds);
   FD_SET(in_fd, &rfds);

   FD_ZERO(&erfds);
   FD_SET(in_fd, &erfds);

   tv.tv_sec = timeout_ms/1000;
   tv.tv_usec = (timeout_ms % 1000) * 1000L;
   select_ret = select(in_fd+1, &rfds, NULL,  &erfds, &tv);
   if(select_ret==-1)
   {
      printf("select_ret=%i\n" , select_ret);
      printf("errno =%d\n", errno);
      fflush(stdout);
      return (FALSE);
   }

   if (FD_ISSET(in_fd, &erfds))
   {
      printf("fd is in error fds\n");
      printf("errno =%d\n", errno);
      fflush(stdout);
      return (FALSE);
   }

   return(FD_ISSET(in_fd,&rfds));
}

static bool IO_Serial_WaitToWrite (unsigned delay_ms, unsigned timeout_ms)
{
   fd_set wfds;
   fd_set ewfds;
   struct timeval tv;
   int select_ret;
   int out_fd;

#ifdef SCI_DEV
   if(reader[ridx].typ == R_INTERNAL)
      return TRUE;
#endif

   if (delay_ms > 0)
	{
#ifdef HAVE_NANOSLEEP
      struct timespec req_ts;

      req_ts.tv_sec = delay_ms / 1000;
      req_ts.tv_nsec = (delay_ms % 1000) * 1000000L;
      nanosleep (&req_ts, NULL);
#else
      usleep (delay_ms * 1000L);
#endif
   }

   out_fd=reader[ridx].handle;

   FD_ZERO(&wfds);
   FD_SET(out_fd, &wfds);

   FD_ZERO(&ewfds);
   FD_SET(out_fd, &ewfds);

   tv.tv_sec = timeout_ms/1000L;
   tv.tv_usec = (timeout_ms % 1000) * 1000L;

   select_ret = select(out_fd+1, NULL, &wfds, &ewfds, &tv);

   if(select_ret==-1)
   {
      printf("select_ret=%d\n" , select_ret);
      printf("errno =%d\n", errno);
      fflush(stdout);
      return (FALSE);
   }

   if (FD_ISSET(out_fd, &ewfds))
   {
      printf("fd is in ewfds\n");
      printf("errno =%d\n", errno);
      fflush(stdout);
      return (FALSE);
   }

   return(FD_ISSET(out_fd,&wfds));

}

static void IO_Serial_Clear (IO_Serial * io)
{
	memset (io->PnP_id, 0, IO_SERIAL_PNPID_SIZE);
	io->PnP_id_size = 0;
	wr = 0;
	//modifyable properties:
	io->input_bitrate = 0;
	io->output_bitrate = 0;
	io->bits = 0;
	io->stopbits = 0;
	io->parity = 0;
	io->dtr = 0;
	io->rts = 0;
}

static bool IO_Serial_InitPnP (IO_Serial * io)
{
	int i = 0;
	io->input_bitrate = 1200;
	io->output_bitrate = 1200;
	io->parity = PARITY_NONE;
	io->bits = 7;
	io->stopbits = 1;
	io->dtr = IO_SERIAL_HIGH;
	io->rts = IO_SERIAL_LOW;

	if (!IO_Serial_SetPropertiesOld (io))
		return FALSE;

	while ((i < IO_SERIAL_PNPID_SIZE) && IO_Serial_Read (200, 1, &(io->PnP_id[i])))
      i++;

	io->PnP_id_size = i;
		return TRUE;
}

