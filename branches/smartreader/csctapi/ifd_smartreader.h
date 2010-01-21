/*
    ifd_smartreader.h
    Header file for Argolis smartreader+.
*/
#if defined(HAVE_LIBUSB) && defined(USE_PTHREAD)
#ifndef __SMARTREADER__
#define __SMARTREADER__

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>

#include <pthread.h>
#include <memory.h>

#include "ftdi.h"
#include "atr.h"


#define LOBYTE(w) ((BYTE)((w) & 0xff))
#define HIBYTE(w) ((BYTE)((w) >> 8))

typedef struct  {
    int F;
    float D;
    int fs;
    int N;
    int T;
    int inv;
} SR_CONFIG;

unsigned char g_read_buffer[4096];
unsigned short g_read_buffer_size;
pthread_mutex_t g_read_mutex;
pthread_mutex_t g_usb_mutex;
struct ftdi_context ftdic;
struct usb_device *smartreader_usb_dev;
pthread_t rt;
unsigned char modem_status;

SR_CONFIG sr_config;


int SR_Init (int device_index);
int SR_Reset (ATR ** atr);
int SR_Transmit (BYTE * buffer, unsigned size);
int SR_Receive (BYTE * buffer, unsigned size);
int SR_SetBaudrate (int mhz);

// bool find_smartreader(int index, struct ftdi_context* ftdic,struct usb_device *dev);
struct usb_device * find_smartreader(int index, struct ftdi_context* ftdic);
void smart_flush(struct ftdi_context* ftdic);
int smart_read(struct ftdi_context* ftdic, unsigned char* buff, size_t size, int timeout_sec);
int smart_write(struct ftdi_context* ftdic, unsigned char* buff, size_t size, int udelay);
void EnableSmartReader(struct ftdi_context* ftdic, int clock, unsigned short Fi, unsigned char Di, unsigned char Ni, unsigned char T,unsigned char inv);
void ResetSmartReader(struct ftdi_context* ftdic);
void* ReaderThread(void *p);
bool smartreader_check_endpoint(struct usb_device *dev);

#endif // __SMARTREADER__
#endif // HAVE_LIBUSB && USE_PTHREAD
