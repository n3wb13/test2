#if defined(HAVE_LIBUSB) && defined(USE_PTHREAD)
/*
		ifd_smartreader.c
		This module provides IFD handling functions for for Argolis smartreader+.
*/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include"ifd_smartreader.h"

#define OK 		1 
#define ERROR 0

int SR_Init (struct s_reader *reader, int device_index)
{
    
    int ret;
    reader->smartreader_usb_dev=NULL;
    if(!(reader->smartreader_usb_dev=find_smartreader(device_index, &reader->ftdic)))
        return ERROR;
    //The smartreader has different endpoint addresses
    //compared to a real FT232 device, so change them here,
    //also a good way to compare a real FT232 with a smartreader
    //if you enumarate usb devices
    reader->ftdic.in_ep = 0x1;
    reader->ftdic.out_ep = 0x82;

    
    //open the smartreader device if found by find_smartreader
    if ((ret = ftdi_usb_open_dev(&reader->ftdic,reader->smartreader_usb_dev)) < 0) {
        cs_log("unable to open ftdi device: %d (%s)", ret, ftdi_get_error_string(&reader->ftdic));
        return ERROR;
    }

#ifdef DEBUG_IO
    cs_log("IO:SR: Setting smartreader latency timer to 1ms");
#endif
    //Set the FTDI latency timer to 1ms
    ret = ftdi_set_latency_timer(&reader->ftdic, 1);

    //Set databits to 8o2
    ret = ftdi_set_line_property(&reader->ftdic, BITS_8, STOP_BIT_2, ODD);

    //Set the DTR HIGH and RTS LOW
    ftdi_setdtr_rts(&reader->ftdic, 1, 1);

    //Disable flow control
    ftdi_setflowctrl(&reader->ftdic, 0);

    // star the reading thread
    reader->g_read_buffer_size = 0;
    reader->modem_status = 0 ;
    pthread_mutex_init(&reader->g_read_mutex,NULL);
    pthread_mutex_init(&reader->g_usb_mutex,NULL);
    ret = pthread_create(&reader->rt, NULL, ReaderThread, (void *)(reader));
    if (ret) {
        cs_log("ERROR; return code from pthread_create() is %d", ret);
        return ERROR;
    }

	return OK;
}


int SR_GetStatus (struct s_reader *reader, int * in)
{
	int state;

    pthread_mutex_lock(&reader->g_read_mutex);
    state =(reader->modem_status & 0x80) == 0x80 ? 0 : 2;
    pthread_mutex_unlock(&reader->g_read_mutex);

    
	//state = 0 no card, 1 = not ready, 2 = ready
	if (state)
		*in = 1; //CARD, even if not ready report card is in, or it will never get activated
	else
		*in = 0; //NOCARD
	return OK;
}

int SR_Reset (struct s_reader *reader, ATR ** atr)
{
    unsigned char data[ATR_MAX_SIZE*2];
    int ret;
    
    memset(data,0,sizeof(data));
    //Reset smartreader to defaults
    ResetSmartReader(reader);

    //Reset smartcard
    pthread_mutex_lock(&reader->g_usb_mutex);
    ftdi_setdtr_rts(&reader->ftdic, 1, 1);
    usleep(200000);
    ftdi_setdtr_rts(&reader->ftdic, 1, 0);
    pthread_mutex_unlock(&reader->g_usb_mutex);
    usleep(2000);
    sched_yield();

    //Read the ATR
    ret = smart_read(reader,data, ATR_MAX_SIZE,1);
#ifdef DEBUG_IO
    cs_log("IO:SR: get ATR ret = %d" , ret);
#endif
    // did we get any data for the ATR ?
    if(!ret)
        return ERROR;

    if(data[0]==0x03) {
        reader->sr_config.inv=1;
        EnableSmartReader(reader, 3571200, 372, 1, 0, 0, reader->sr_config.inv);
    }
    // parse atr
	(*atr) = ATR_New ();
	if(ATR_InitFromArray ((*atr), data, ret) == ATR_OK)
	{
		struct timespec req_ts;
		req_ts.tv_sec = 0;
		req_ts.tv_nsec = 50000000;
		nanosleep (&req_ts, NULL);
		return OK;
	}
	else
	{
		ATR_Delete (*atr);
		(*atr) = NULL;
		return ERROR;
	}
    
}

int SR_Transmit (struct s_reader *reader, BYTE * buffer, unsigned size)

{ 
    unsigned int ret;
    ret = smart_write(reader, buffer, size, 0);
    if (ret!=size)
        return ERROR;
        
	return OK;
}

int SR_Receive (struct s_reader *reader, BYTE * buffer, unsigned size)
{ 
    unsigned int ret;
    ret = smart_read(reader, buffer, size, 1);
    if (ret!=size)
        return ERROR;

	return OK;
}	

int SR_SetBaudrate (struct s_reader *reader)
{
    reader->sr_config.fs=reader->mhz*10000; //freq in KHz
    EnableSmartReader(reader, reader->sr_config.fs, reader->sr_config.F, (BYTE)reader->sr_config.D, reader->sr_config.N, reader->sr_config.T, reader->sr_config.inv);
    //baud rate not really used in native mode since
    //it's handled by the card, so just set to maximum 3Mb/s
    pthread_mutex_lock(&reader->g_usb_mutex);
    ftdi_set_baudrate(&reader->ftdic, 3000000);
    pthread_mutex_unlock(&reader->g_usb_mutex);
    sched_yield();

	return OK;
}


static struct usb_device * find_smartreader(int index, struct ftdi_context* ftdic)
{
    int i;
    bool dev_found;
    struct usb_bus *bus;
    struct usb_device *dev;

    if (ftdi_init(ftdic) < 0) {
        cs_log("ftdi_init failed");
        return NULL;
    }
    usb_init();
    if (usb_find_busses() < 0) {
        cs_log("usb_find_busses() failed");
        return NULL;
    }
    if (usb_find_devices() < 0) {
        cs_log("usb_find_devices() failed");
        return NULL;
    }


    i = -1;
    dev_found=FALSE;
    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if ( (dev->descriptor.idVendor != 0x0403) || (dev->descriptor.idProduct != 0x6001))
                    continue;
#ifdef DEBUG_IO
            cs_log("IO:SR: Checking FTDI device: %d", i+1);
#endif
            if(smartreader_check_endpoint(dev)) {
                i++;
                if (i==index) {
                    dev_found=TRUE;
                    break;
                }
            }
        }
        if(dev_found) {
#ifdef DEBUG_IO
            cs_log("IO:SR: Found smartreader device %d",index);
#endif
            break;
        }
    }

    if(!dev_found) {
        cs_log("IO:SR: Smartreader device number %d not found",index);
        ftdi_deinit(ftdic);
        return NULL;
        }

    return dev;
}

static void smart_flush(struct s_reader *reader)
{

    pthread_mutex_lock(&reader->g_usb_mutex);
    ftdi_usb_purge_buffers(&reader->ftdic);
    pthread_mutex_unlock(&reader->g_usb_mutex);

    pthread_mutex_lock(&reader->g_read_mutex);
    reader->g_read_buffer_size = 0;
    pthread_mutex_unlock(&reader->g_read_mutex);
    usleep(500);
    sched_yield();
}

static unsigned int smart_read(struct s_reader *reader, unsigned char* buff, size_t size, int timeout_sec)
{

    int ret = 0;
    unsigned int total_read = 0;
    struct timeval start, now, dif = {0,0};
    gettimeofday(&start,NULL);


    while(total_read < size && dif.tv_sec < timeout_sec) {

        pthread_mutex_lock(&reader->g_read_mutex);
        if(reader->g_read_buffer_size > 0) {
        
            ret = reader->g_read_buffer_size > size-total_read ? size-total_read : reader->g_read_buffer_size;
            memcpy(buff+total_read,reader->g_read_buffer,ret);
            reader->g_read_buffer_size -= ret;
            total_read+=ret;
        }
        pthread_mutex_unlock(&reader->g_read_mutex);
       
        gettimeofday(&now,NULL);
        timersub(&now, &start, &dif);
        usleep(500);
        sched_yield();
    }

    
    return total_read;
}

static unsigned int smart_write(struct s_reader *reader, unsigned char* buff, size_t size, int udelay)
{

    int ret = 0;
    unsigned int idx;

    if (udelay == 0) {
        pthread_mutex_lock(&reader->g_usb_mutex);
        ret = ftdi_write_data(&reader->ftdic, buff, size);
        pthread_mutex_unlock(&reader->g_usb_mutex);
    }
    else {
        for (idx = 0; idx < size; idx++) {
            pthread_mutex_lock(&reader->g_usb_mutex);
            if ((ret = ftdi_write_data(&reader->ftdic, &buff[idx], 1)) < 0){
                pthread_mutex_unlock(&reader->g_usb_mutex);
                break;
            }
            pthread_mutex_unlock(&reader->g_usb_mutex);
            usleep(udelay);
        }
    }
    usleep(500);
    sched_yield();
    return ret;
}

static void EnableSmartReader(struct s_reader *reader, int clock, unsigned short Fi, unsigned char Di, unsigned char Ni, unsigned char T, unsigned char inv) {

    int ret = 0;
    int delay=50000;

    
    pthread_mutex_lock(&reader->g_usb_mutex);
    ret = ftdi_set_baudrate(&reader->ftdic, 9600);
    ret = ftdi_set_line_property(&reader->ftdic, (enum ftdi_bits_type) 5, STOP_BIT_2, NONE);
    pthread_mutex_unlock(&reader->g_usb_mutex);
#ifdef DEBUG_IO
    cs_log("IO:SR: sending F=%04X to smartreader",Fi);
    cs_log("IO:SR: sending D=%02X to smartreader",Di);
#endif

    unsigned char FiDi[] = {0x01, HIBYTE(Fi), LOBYTE(Fi), Di};
    ret = smart_write(reader,FiDi, sizeof (FiDi),0);
    usleep(delay);

    unsigned short freqk = (unsigned short) (clock / 1000);
#ifdef DEBUG_IO
    cs_log("IO:SR: sending Freq=%d to smartreader",freqk);
#endif
    unsigned char Freq[] = {0x02, HIBYTE(freqk), LOBYTE(freqk)};
    ret = smart_write(reader, Freq, sizeof (Freq),0);
    usleep(delay);

#ifdef DEBUG_IO
    cs_log("IO:SR: sending N=%02X to smartreader",Ni);
#endif
    unsigned char N[] = {0x03, Ni};
    ret = smart_write(reader, N, sizeof (N),0);
    usleep(delay);

#ifdef DEBUG_IO
    cs_log("IO:SR: sending T=%02X to smartreader",T);
#endif
    unsigned char Prot[] = {0x04, T};
    ret = smart_write(reader, Prot, sizeof (Prot),0);
    usleep(delay);

#ifdef DEBUG_IO
    cs_log("IO:SR: sending inv=%02X to smartreader",inv);
#endif
    unsigned char Invert[] = {0x05, inv};
    ret = smart_write(reader, Invert, sizeof (Invert),0);
    usleep(delay);

    pthread_mutex_lock(&reader->g_usb_mutex);
    ret = ftdi_set_line_property2(&reader->ftdic, BITS_8, STOP_BIT_2, ODD, BREAK_ON);
    pthread_mutex_unlock(&reader->g_usb_mutex);
   
    usleep(50000);

    pthread_mutex_lock(&reader->g_usb_mutex);
    ret = ftdi_set_line_property2(&reader->ftdic, BITS_8, STOP_BIT_2, ODD, BREAK_OFF);
    pthread_mutex_unlock(&reader->g_usb_mutex);

    smart_flush(reader);
}

static void ResetSmartReader(struct s_reader *reader) 
{

    smart_flush(reader);
    // set smartreader+ default values 
    reader->sr_config.F=372; 
    reader->sr_config.D=1.0; 
    reader->sr_config.fs=3571200; 
    reader->sr_config.N=0; 
    reader->sr_config.T=0; 
    reader->sr_config.inv=0; 

    EnableSmartReader(reader, reader->sr_config.fs, reader->sr_config.F, (BYTE)reader->sr_config.D, reader->sr_config.N, reader->sr_config.T, reader->sr_config.inv);
    sched_yield();

}

static void* ReaderThread(void *p)
{

    struct s_reader *reader;
    bool running = TRUE;
    unsigned int ret;
    unsigned int copy_size;
    unsigned char local_buffer[64];  //64 is max transfer size of FTDI bulk pipe

    reader = (struct s_reader *)p;

    while(running){

        if(reader->g_read_buffer_size == sizeof(reader->g_read_buffer)){
            //if out read buffer is full then delay
            //slightly and go around again
            struct timespec req = {0,200000000}; //20ms
            nanosleep(&req,NULL);
            continue;
        }

        pthread_mutex_lock(&reader->g_usb_mutex);
        ret = usb_bulk_read(reader->ftdic.usb_dev,reader->ftdic.out_ep,(char*)local_buffer,64,1000);
        pthread_mutex_unlock(&reader->g_usb_mutex);
        sched_yield();


        if(ret>2) {  //FTDI always sends modem status bytes as first 2 chars with the 232BM
            pthread_mutex_lock(&reader->g_read_mutex);
            reader->modem_status=local_buffer[0];

            copy_size = sizeof(reader->g_read_buffer) - reader->g_read_buffer_size > ret-2 ?ret-2: sizeof(reader->g_read_buffer) - reader->g_read_buffer_size;
            memcpy(reader->g_read_buffer+reader->g_read_buffer_size,local_buffer+2,copy_size);
            reader->g_read_buffer_size += copy_size;            
            pthread_mutex_unlock(&reader->g_read_mutex);
        } 
        else {
            if(ret==2) {
                pthread_mutex_lock(&reader->g_read_mutex);
                reader->modem_status=local_buffer[0];
                pthread_mutex_unlock(&reader->g_read_mutex);
            }
            //sleep for 50ms since there was nothing to read last time
            usleep(50000);
        }
    }

    pthread_exit(NULL);
}


static bool smartreader_check_endpoint(struct usb_device *dev)
{
    int nb_interfaces;
    int i,j,k,l;
    u_int8_t tmpEndpointAddress;
    int nb_endpoint_ok;

    if (!dev->config) {
#ifdef DEBUG_IO
        cs_log("IO:SR:  Couldn't retrieve descriptors");
#endif
        return FALSE;
    }
        
    nb_interfaces=dev->config->bNumInterfaces;
    // smartreader only has 1 interface
    if(nb_interfaces!=1) {
#ifdef DEBUG_IO
        cs_log("IO:SR:  Couldn't retrieve interfaces");
#endif
        return FALSE;
    }

    nb_endpoint_ok=0;
    for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
        for (j = 0; j < dev->config[i].bNumInterfaces; j++)
            for (k = 0; k < dev->config[i].interface[j].num_altsetting; k++)
                for (l = 0; l < dev->config[i].interface[j].altsetting[k].bNumEndpoints; l++) {
                    tmpEndpointAddress=dev->config[i].interface[j].altsetting[k].endpoint[l].bEndpointAddress;
#ifdef DEBUG_IO
                    cs_log("IO:SR:  checking endpoint address %02X",tmpEndpointAddress);
#endif
                    if((tmpEndpointAddress== 0x1) || (tmpEndpointAddress== 0x82))
                        nb_endpoint_ok++;
                }

    if(nb_endpoint_ok!=2)
        return FALSE;
    return TRUE;
}

#endif // HAVE_LIBUSB && USE_PTHREAD
