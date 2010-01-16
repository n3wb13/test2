/*
    ifd_towitoko.c
    This module provides IFD handling functions.

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include <sys/time.h>
#endif
#include <sys/ioctl.h>
#include <time.h>
#include "ifd_towitoko.h"
#include "io_serial.h"
#include "sci_global.h"
#include "sci_ioctl.h"
#include "ifd.h"
#include "../globals.h"

/*
 * Not exported constants
 */

#define IFD_TOWITOKO_TIMEOUT             1000
#define IFD_TOWITOKO_DELAY               0
#define IFD_TOWITOKO_BAUDRATE            9600
//#define IFD_TOWITOKO_PS                  15
#define IFD_TOWITOKO_MAX_TRANSMIT        255
//#define IFD_TOWITOKO_ATR_TIMEOUT	 200
//#define IFD_TOWITOKO_ATR_TIMEOUT	 400
#define IFD_TOWITOKO_ATR_TIMEOUT	 800
#define IFD_TOWITOKO_ATR_MIN_LENGTH      1
#define IFD_TOWITOKO_CLOCK_RATE          (625L * 9600L)
//#define IFD_TOWITOKO_CLOCK_RATE          (372L * 9600L)

#define HI(a) 				(((a) & 0xff00) >> 8)
#define LO(a) 				((a) & 0x00ff)

/*
 * Not exported functions declaration
 */

static int IFD_Towitoko_GetReaderInfo (IFD * ifd);
static void IFD_Towitoko_Clear (IFD * ifd);

#ifdef USE_GPIO

int gpio_outen,gpio_out,gpio_in;
unsigned int pin,gpio;
int gpio_detect=0;

static void set_gpio(int level)
{     
	read(gpio_outen, &gpio, sizeof(gpio));
	gpio |= pin; 
	write(gpio_outen, &gpio, sizeof(gpio));

	read(gpio_out, &gpio, sizeof(gpio));
	if (level>0)
		gpio|=pin;
	else
		gpio&=~pin;
	write(gpio_out, &gpio, sizeof(gpio));
}

static void set_gpio1(int level)
{     
	read(gpio_outen, &gpio, sizeof(gpio));
	gpio |= 2; 
	write(gpio_outen, &gpio, sizeof(gpio));

	read(gpio_out, &gpio, sizeof(gpio));
	if (level>0)
		gpio|=2;
	else
		gpio&=~2;
	write(gpio_out, &gpio, sizeof(gpio));
}

static void set_gpio_input(void)
{
	read(gpio_outen, &gpio, sizeof(gpio));      
	gpio &= ~pin;
	write(gpio_outen, &gpio, sizeof(gpio));
}

static int get_gpio(void)
{
	set_gpio_input();
	read(gpio_in, &gpio, sizeof(gpio));
	return ((int)((gpio&pin)?1:0));
}
#endif

/*
 * Exported functions definition
 */

IFD * IFD_Towitoko_New ()
{
	IFD *ifd;
	
	ifd = (IFD *) malloc (sizeof (IFD));
	
	if (ifd != NULL)
		IFD_Towitoko_Clear (ifd);
	
	return ifd;
}

void IFD_Towitoko_Delete (IFD * ifd)
{
	free (ifd);
}

int IFD_Towitoko_Init (IFD * ifd, IO_Serial * io, BYTE slot)
{
	int ret;

#ifdef USE_GPIO	     	
	extern int oscam_card_detect;
	if (oscam_card_detect>4)
	{
		gpio_detect=oscam_card_detect-3;
		pin = 1<<gpio_detect;
		gpio_outen=open("/dev/gpio/outen",O_RDWR);
		gpio_out=open("/dev/gpio/out",O_RDWR);
		gpio_in=open("/dev/gpio/in",O_RDWR); 
		set_gpio_input();
	}
#endif
	
#ifdef DEBUG_IFD
	printf ("IFD: Initializing slot number %d, com=%d\n", slot, reader[ridx].typ);
#endif
	
//	if ((slot != IFD_TOWITOKO_SLOT_MULTICAM) && (slot != IFD_TOWITOKO_SLOT_A) && (slot != IFD_TOWITOKO_SLOT_B))
	if (slot != IFD_TOWITOKO_SLOT_MULTICAM )
		return IFD_TOWITOKO_PARAM_ERROR;
		
	if(reader[ridx].typ == R_INTERNAL)
	{
		ifd->io = io;
		ifd->slot = slot;
		ifd->type = IFD_TOWITOKO_MULTICAM;
		return IFD_TOWITOKO_OK;
	}

	
	/* Default serial port settings */
	if (!IO_Serial_SetParams (IFD_TOWITOKO_BAUDRATE, 8, PARITY_EVEN, 2, IO_SERIAL_HIGH, IO_SERIAL_LOW))
		return FALSE;
		
	/* Default ifd settings */
	
	ifd->io = io;
	ifd->slot = slot;
	ifd->type = IFD_TOWITOKO_MULTICAM;
	
	if (!Phoenix_SetBaudrate(IFD_TOWITOKO_BAUDRATE))
	{
		IFD_Towitoko_Clear (ifd);
		return IFD_TOWITOKO_IO_ERROR;
	}
	
	if (!IO_Serial_SetParity (PARITY_EVEN))
		return IFD_TOWITOKO_IO_ERROR;
	
	if (ret != IFD_TOWITOKO_OK)
	{
		IFD_Towitoko_Clear (ifd);
		return ret;
	}
	
	ret = IFD_Towitoko_GetReaderInfo (ifd);
		
	if (ret != IFD_TOWITOKO_OK)
	{
		IFD_Towitoko_Clear (ifd);
	}
	else
	{	
		IO_Serial_Flush(ifd->io);
	}
	
	return ret;
}

int IFD_Towitoko_Close (IFD * ifd)
{
	int ret;

#ifdef USE_GPIO
	if(gpio_detect) 
	{
		close(gpio_outen);
		close(gpio_out);
		close(gpio_in);
       }
#endif
	
#ifdef DEBUG_IFD
	printf ("IFD: Closing slot number %d\n", ifd->slot);
#endif
	
	IFD_Towitoko_Clear (ifd);
	
	
	return IFD_TOWITOKO_OK;
}

int IFD_Towitoko_ActivateICC (IFD * ifd)
{
#ifdef DEBUG_IFD
		printf ("IFD: Activating card\n");
#endif
#ifdef SCI_DEV
	if(reader[ridx].typ == R_INTERNAL)
	{
		int in;

#if defined(TUXBOX) && (defined(MIPSEL) || defined(PPC) || defined(SH4))
		if(ioctl(reader[ridx].handle, IOCTL_GET_IS_CARD_PRESENT, &in)<0)
#else
		if(ioctl(reader[ridx].handle, IOCTL_GET_IS_CARD_ACTIVATED, &in)<0)
#endif
			return IFD_TOWITOKO_IO_ERROR;
			
		if(in)
		{
			struct timespec req_ts;
			req_ts.tv_sec = 0;
			req_ts.tv_nsec = 50000000;
			nanosleep (&req_ts, NULL);
			return IFD_TOWITOKO_OK;
		}
		else
		{
			return IFD_TOWITOKO_IO_ERROR;
		}
	}
	else
#endif
	{
		return IFD_TOWITOKO_OK;
	}
}

int IFD_Towitoko_DeactivateICC (IFD * ifd)
{
#ifdef DEBUG_IFD
		printf ("IFD: Deactivating card\n");
#endif

#ifdef SCI_DEV
	if(reader[ridx].typ == R_INTERNAL)
	{
		int in;
		
#if defined(TUXBOX) && (defined(MIPSEL) || defined(PPC) || defined(SH4))
		if(ioctl(reader[ridx].handle, IOCTL_GET_IS_CARD_PRESENT, &in)<0)
#else
		if(ioctl(reader[ridx].handle, IOCTL_GET_IS_CARD_ACTIVATED, &in)<0)
#endif
			return IFD_TOWITOKO_IO_ERROR;
			
		if(in)
		{
			if(ioctl(reader[ridx].handle, IOCTL_SET_DEACTIVATE)<0)
				return IFD_TOWITOKO_IO_ERROR;
		}
		
		
	}
#endif
	
	return IFD_TOWITOKO_OK;
}

//extern void print_hex_data(unsigned char *data, int len);

int IFD_Towitoko_ResetAsyncICC (IFD * ifd, ATR ** atr)
{	 

#ifdef DEBUG_IFD
	printf ("IFD: Resetting card:\n");
#endif

		int ret;
		int parity;
		int i;
		int par[3] = {PARITY_EVEN, PARITY_ODD, PARITY_NONE};
#ifdef HAVE_NANOSLEEP
		struct timespec req_ts;
		req_ts.tv_sec = 0;
		req_ts.tv_nsec = 50000000;
#endif
		
		(*atr) = NULL;
		for(i=0; i<3; i++)
		{
			parity = par[i];
			IO_Serial_Flush();

			if (!IO_Serial_SetParity (parity))
				return IFD_TOWITOKO_IO_ERROR;

			ret = IFD_TOWITOKO_IO_ERROR;

			IO_Serial_Ioctl_Lock(1);
#ifdef USE_GPIO
			if (gpio_detect)
			{
				set_gpio(0);
				set_gpio1(0);
			}
			else
#endif
				IO_Serial_RTS_Set();

#ifdef HAVE_NANOSLEEP
			nanosleep (&req_ts, NULL);
#else
			usleep (50000L);
#endif
#ifdef USE_GPIO
			if (gpio_detect)
			{
				set_gpio_input();
				set_gpio1(1);
			}
			else
#endif
				IO_Serial_RTS_Clr();
			
			IO_Serial_Ioctl_Lock(0);

			(*atr) = ATR_New ();

			if(ATR_InitFromStream ((*atr), IFD_TOWITOKO_ATR_TIMEOUT) == ATR_OK)
				ret = IFD_TOWITOKO_OK;

			/* Succesfully retrive ATR */
			if (ret == IFD_TOWITOKO_OK)
			{			
				break;
			}
			else
			{
				ATR_Delete (*atr);
				(*atr) = NULL;
#ifdef USE_GPIO
				if (gpio_detect) set_gpio1(0);
#endif
			}	
		}
	
		IO_Serial_Flush();

/*
		//PLAYGROUND faking ATR for test purposes only
		//
  		// sky 919 unsigned char atr_test[] = { 0x3F, 0xFF, 0x13, 0x25, 0x03, 0x10, 0x80, 0x33, 0xB0, 0x0E, 0x69, 0xFF, 0x4A, 0x50, 0x70, 0x00, 0x00, 0x49, 0x54, 0x02, 0x00, 0x00 };
  		// HD+ unsigned char atr_test[] = { 0x3F, 0xFF, 0x95, 0x00, 0xFF, 0x91, 0x81, 0x71, 0xFE, 0x47, 0x00, 0x44, 0x4E, 0x41, 0x53, 0x50, 0x31, 0x34, 0x32, 0x20, 0x52, 0x65, 0x76, 0x47, 0x43, 0x34, 0x63 };
		// S02 = irdeto unsigned char atr_test[] = { 0x3B, 0x9F, 0x21, 0x0E, 0x49, 0x52, 0x44, 0x45, 0x54, 0x4F, 0x20, 0x41, 0x43, 0x53, 0x03};
		//cryptoworks 	unsigned char atr_test[] = { 0x3B, 0x78, 0x12, 0x00, 0x00, 0x65, 0xC4, 0x05, 0xFF, 0x8F, 0xF1, 0x90, 0x00 };
		ATR_Delete(*atr); //throw away actual ATR
		(*atr) = ATR_New ();
		ATR_InitFromArray ((*atr), atr_test, sizeof(atr_test));
		//END OF PLAYGROUND 
*/
		
		return ret;
}

BYTE IFD_Towitoko_GetType (IFD * ifd)
{
	return ifd->type;
}

void IFD_Towitoko_GetDescription (IFD * ifd, BYTE * desc, unsigned length)
{
	char buffer[3];

	if (ifd->type == IFD_TOWITOKO_CHIPDRIVE_EXT_II)
		memcpy (desc,"CE2",MIN(length,3));

	else if  (ifd->type == IFD_TOWITOKO_CHIPDRIVE_EXT_I)
		memcpy (desc,"CE1",MIN(length,3));

	else if (ifd->type == IFD_TOWITOKO_CHIPDRIVE_INT)
		memcpy (desc,"CDI",MIN(length,3));

	else if (ifd->type == IFD_TOWITOKO_CHIPDRIVE_MICRO)
		memcpy (desc,"CDM",MIN(length,3));

	else if (ifd->type == IFD_TOWITOKO_KARTENZWERG_II) 
		memcpy (desc,"KZ2",MIN(length,3));

	else if (ifd->type == IFD_TOWITOKO_KARTENZWERG)
		memcpy (desc,"KZ1",MIN(length,3));
		
	else if (ifd->type == IFD_TOWITOKO_MULTICAM)
		memcpy (desc,"MCM",MIN(length,3));

	else 
		memcpy (desc,"UNK",MIN(length,3));
	
	snprintf (buffer, 3, "%02X", ifd->firmware);

	if (length > 3)
		memcpy (desc+3, buffer, MIN(length-3,2));
}

BYTE
IFD_Towitoko_GetFirmware (IFD * ifd)
{
	return ifd->firmware;
}

BYTE
IFD_Towitoko_GetSlot (IFD * ifd)
{
	return ifd->slot;
}

unsigned
IFD_Towitoko_GetNumSlots ()
{
	return 1;
}

/*
 * Not exported funcions definition
 */


static int IFD_Towitoko_GetReaderInfo (IFD * ifd)
{
	BYTE status[3];
	
	status[0] = IFD_TOWITOKO_MULTICAM;
	status[1] = 0x00;  	
	
	ifd->type = status[0];
	ifd->firmware = status[1];
	
#ifdef DEBUG_IFD
	printf ("IFD: Reader type = %s\n",
	status[0] == IFD_TOWITOKO_CHIPDRIVE_EXT_II ? "Chipdrive Extern II" :
	status[0] == IFD_TOWITOKO_CHIPDRIVE_EXT_I ? "Chipdrive Extern I" :
	status[0] == IFD_TOWITOKO_CHIPDRIVE_INT ? "Chipdrive Intern" :
	status[0] == IFD_TOWITOKO_CHIPDRIVE_MICRO ? "Chipdrive Micro" :
	status[0] == IFD_TOWITOKO_KARTENZWERG_II ? "Kartenzwerg II" :
	status[0] == IFD_TOWITOKO_MULTICAM ? "Multicam" : 
	status[0] == IFD_TOWITOKO_KARTENZWERG ? "Kartenzwerg" : "Unknown");
#endif
	
	return IFD_TOWITOKO_OK;
}


static void IFD_Towitoko_Clear (IFD * ifd)
{
	ifd->io = NULL;
	ifd->slot = 0x00;
	ifd->type = 0x00;
	ifd->firmware = 0x00;
	reader[ridx].status = 0;
}
