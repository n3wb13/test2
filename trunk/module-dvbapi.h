#ifdef HAVE_DVBAPI

#ifndef MODULEDVBAPI_H_
#define MODULEDVBAPI_H_


#include <sys/un.h>
#include <dirent.h>

#define TYPE_ECM 1
#define TYPE_EMM 2

//api
#define DVBAPI_3	0
#define DVBAPI_1	1
#define STAPI		2
#define COOLAPI		3

#define PORT		9000

#define TMPDIR	"/tmp/"
#define STANDBY_FILE	"/tmp/.pauseoscam"
#define ECMINFO_FILE	"/tmp/ecm.info"

#ifdef COOL
#define MAX_CA_DEVICES 4
#endif
#define MAX_DEMUX 16
#define MAX_CAID 50
#define ECM_PIDS 30
#define MAX_FILTER 24

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define BOX_COUNT 6

struct box_devices
{
	char *path;
	char *ca_device;
	char *demux_device;
	char *cam_socket_path;
	int8_t api;
};

struct s_ecmpids
{
	uint16_t CAID;
	uint32_t PROVID;
	uint16_t ECM_PID;
	uint16_t EMM_PID;
	int8_t irdeto_numchids;
	int8_t irdeto_curchid;
	int32_t irdeto_chids;
	int32_t irdeto_cycle;
	int8_t checked;
	int8_t status;
	unsigned char table;
	int8_t index;
	uint32_t streams;
};

typedef struct filter_s
{
	uint32_t fd; //FilterHandle
	int32_t pidindex;
	int32_t pid;
	uint16_t caid;
	uint16_t type;
	int32_t count;
#ifdef WITH_STAPI
	int32_t NumSlots;
	uint32_t	SlotHandle[10];
	uint32_t  	BufferHandle[10];
#endif
} FILTERTYPE;

struct s_emmpids
{
	uint16_t CAID;
	uint32_t PROVID;
	uint16_t PID;
	uint8_t type;
};

#define PTINUM 10
#define SLOTNUM 20

typedef struct demux_s
{
	int8_t demux_index;
	FILTERTYPE demux_fd[MAX_FILTER];
	int32_t ca_mask;
	int8_t adapter_index;
	int32_t socket_fd;
	int8_t ECMpidcount;
	struct s_ecmpids ECMpids[ECM_PIDS];
	int8_t EMMpidcount;
	struct s_emmpids EMMpids[ECM_PIDS];
	int8_t STREAMpidcount;
	uint16_t STREAMpids[ECM_PIDS];
	int16_t pidindex;
	int16_t curindex;
	int8_t tries;
	int8_t max_status;
	uint16_t program_number;
	unsigned char lastcw[2][8];
	int8_t emm_filter;
	uchar hexserial[8];
	struct s_reader *rdr;
	char pmt_file[30];
	int32_t pmt_time;
#ifdef WITH_STAPI
	uint32_t DescramblerHandle[PTINUM];
	int32_t desc_pidcount;
	uint32_t slot_assc[PTINUM][SLOTNUM];
#endif
} DEMUXTYPE;

#ifdef COOL
struct cool_dmx
{
	int32_t		opened;
	int32_t		filter_attached;
	int32_t		fd;
	unsigned char   buffer[4096];
	void *		buffer1;
	void *		buffer2;
	void *	 	channel;
	void *		filter;
	unsigned char   filter16[16];
	unsigned char   mask16[16];
	void *		device;
	int32_t		pid;
	pthread_mutex_t	mutex;
	int32_t 	demux_id;
	int32_t 	demux_index;
	int32_t 	filter_num;
};
typedef struct cool_dmx dmx_t;

typedef struct
{
        int32_t  type;
        uint32_t size;
        int32_t unknown1;
        int16_t unknown2;
        int32_t unknown3;
	int32_t unknown[4];
} buffer_open_arg_t;

typedef struct
{
   int32_t type;
   int32_t unknown[2];
} channel_open_arg_t;

typedef struct
{
	uint32_t number;
	int32_t unknown1;
	int32_t unknown2;
	int32_t unknown3;
	int32_t unknown4;
	int32_t unknown5;
	int32_t unknown6;
	int32_t unknown[6];
} device_open_arg_t;

typedef struct
{
   uint32_t length;
   unsigned char filter[12];
   unsigned char mask[12];
   int32_t unknown[16];
} filter_set_t;

typedef struct
{
   int32_t unk;
   int32_t type;
   int32_t unknown[4];
   uint32_t len;
} dmx_callback_data_t;
#endif

struct s_dvbapi_priority
{
	char type; // p or i
	uint16_t caid;
	uint32_t provid;
	uint16_t srvid;
	uint16_t chid;
	uint16_t ecmpid;
	uint16_t mapcaid;
	uint32_t mapprovid;
	int16_t delay;
	int8_t force;
#ifdef WITH_STAPI
	char devname[30];
	char pmtfile[30];
	int8_t disablefilter;
#endif
	struct s_dvbapi_priority *next;
};


#define DMX_FILTER_SIZE 16


//dvbapi 1
typedef struct dmxFilter
{
	uint8_t 	filter[DMX_FILTER_SIZE];
	uint8_t 	mask[DMX_FILTER_SIZE];
} dmxFilter_t;

struct dmxSctFilterParams
{
	uint16_t		    pid;
	dmxFilter_t		     filter;
	uint32_t		     timeout;
	uint32_t		     flags;
#define DMX_CHECK_CRC	    1
#define DMX_ONESHOT	    2
#define DMX_IMMEDIATE_START 4
#define DMX_BUCKET	    0x1000	/* added in 2005.05.18 */
#define DMX_KERNEL_CLIENT   0x8000
};

#define DMX_START1		  _IOW('o',41,int)
#define DMX_STOP1		  _IOW('o',42,int)
#define DMX_SET_FILTER1 	  _IOW('o',43,struct dmxSctFilterParams *)
//------------------------------------------------------------------


//dbox2+ufs
typedef struct dmx_filter
{
	uint8_t  filter[DMX_FILTER_SIZE];
	uint8_t  mask[DMX_FILTER_SIZE];
	uint8_t  mode[DMX_FILTER_SIZE];
} dmx_filter_t;


struct dmx_sct_filter_params
{
	uint16_t	    pid;
	dmx_filter_t	    filter;
	uint32_t	    timeout;
	uint32_t	    flags;
#define DMX_CHECK_CRC	    1
#define DMX_ONESHOT	    2
#define DMX_IMMEDIATE_START 4
#define DMX_KERNEL_CLIENT   0x8000
};

typedef struct ca_descr {
	uint32_t index;
	uint32_t parity;	/* 0 == even, 1 == odd */
	unsigned char cw[8];
} ca_descr_t;

typedef struct ca_pid {
	uint32_t pid;
	int32_t index;		/* -1 == disable*/
} ca_pid_t;

#define DMX_START		_IO('o', 41)
#define DMX_STOP		_IO('o', 42)
#define DMX_SET_FILTER	_IOW('o', 43, struct dmx_sct_filter_params)

#define CA_SET_DESCR		_IOW('o', 134, ca_descr_t)
#define CA_SET_PID		_IOW('o', 135, ca_pid_t)
// --------------------------------------------------------------------

#ifdef AZBOX
#include "openxcas/openxcas_api.h"
#include "openxcas/openxcas_message.h"

int32_t openxcas_provid, openxcas_seq, openxcas_filter_idx, openxcas_stream_id, openxcas_cipher_idx, openxcas_busy;
unsigned char openxcas_cw[16];
uint16_t openxcas_sid, openxcas_caid, openxcas_ecm_pid, openxcas_video_pid, openxcas_audio_pid, openxcas_data_pid;

void azbox_openxcas_ecm_callback(int32_t stream_id, uint32_t sequence, int32_t cipher_index, uint32_t caid, unsigned char *ecm_data, int32_t l, uint16_t pid);
void azbox_openxcas_ex_callback(int32_t stream_id, uint32_t seq, int32_t idx, uint32_t pid, unsigned char *ecm_data, int32_t l);
void azbox_send_dcw(struct s_client *client, ECM_REQUEST *er);
void * azbox_main(void * cli);
#endif

#ifdef COOL
int32_t cnxt_cbuf_init(void *);
int32_t cnxt_cbuf_open(void **handle, buffer_open_arg_t * arg, void *, void *);
int32_t cnxt_cbuf_get_used(void *buffer, uint32_t * bytes_used);
int32_t cnxt_cbuf_attach(void *handle, int32_t type, void * channel);
int32_t cnxt_cbuf_detach(void *handle, int32_t type, void * channel);
int32_t cnxt_cbuf_close(void * handle);
int32_t cnxt_cbuf_read_data(void * handle, void *buffer, uint32_t size, uint32_t * ret_size);
int32_t cnxt_cbuf_flush(void * handle, int);

void cnxt_kal_initialize(void);
void cnxt_kal_terminate(void);
void cnxt_drv_init(void);
void cnxt_drv_term(void);

int32_t cnxt_dmx_init(void *);
int32_t cnxt_dmx_open(void **device, device_open_arg_t *arg, void *, void *);
int32_t cnxt_dmx_close(void * handle);
int32_t cnxt_dmx_channel_open(void * device, void **channel, channel_open_arg_t * arg, void * callback, void *);
int32_t cnxt_dmx_channel_close(void * channel);
int32_t cnxt_dmx_open_filter(void * handle, void *flt); 
int32_t cnxt_dmx_set_filter(void * handle, filter_set_t * arg, void *);
int32_t cnxt_dmx_close_filter(void * filter);
int32_t cnxt_dmx_channel_attach(void * channel, int32_t param1, int32_t param2, void * buffer);
int32_t cnxt_dmx_channel_detach(void * channel, int32_t param1, int32_t param2, void * buffer);
int32_t cnxt_dmx_channel_attach_filter(void * channel, void * filter);
int32_t cnxt_dmx_channel_detach_filter(void * channel, void * filter);
int32_t cnxt_dmx_set_channel_buffer(void * channel, int32_t param1, void * buffer);
int32_t cnxt_dmx_set_channel_pid(void * channel, uint32_t pid);
int32_t cnxt_dmx_get_channel_from_pid(void * device, uint16_t pid, void * channel);
int32_t cnxt_dmx_set_channel_key(void * channel, int32_t param1, uint32_t parity, unsigned char *cw, uint32_t len);
int32_t cnxt_dmx_channel_ctrl(void * channel, int32_t param1, int32_t param2);

int32_t cnxt_smc_init(void *);


void coolapi_open();
void coolapi_open_all();
void coolapi_close_all();
int32_t coolapi_set_filter (int32_t fd, int32_t num, int32_t pid, unsigned char * flt, unsigned char * mask);
int32_t coolapi_remove_filter (int32_t fd, int32_t num);
int32_t coolapi_open_device (int32_t demux_index, int32_t demux_id);
int32_t coolapi_close_device(int32_t fd);
int32_t coolapi_read(dmx_t * dmx, uint32_t len);
int32_t coolapi_write_cw(int32_t mask, uint16_t *STREAMpids, int32_t count, ca_descr_t * ca_descr);
int32_t coolapi_set_pid (int32_t demux_id, int32_t num, int32_t index, int32_t pid);
void coolapi_close_all();
#endif

void dvbapi_stop_descrambling(int);
void dvbapi_process_input(int32_t demux_id, int32_t filter_num, uchar *buffer, int32_t len);
int32_t dvbapi_open_device(int32_t, int32_t, int);
int32_t dvbapi_stop_filternum(int32_t demux_index, int32_t num);
int32_t dvbapi_stop_filter(int32_t demux_index, int32_t type);
struct s_dvbapi_priority *dvbapi_check_prio_match(int32_t demux_id, int32_t pidindex, char type);
void dvbapi_send_dcw(struct s_client *client, ECM_REQUEST *er);
void dvbapi_write_cw(int32_t demux_id, uchar *cw, int32_t index);

#undef cs_log
#define cs_log(txt, x...)	cs_log_int(0, 1, NULL, 0, "dvbapi: "txt, ##x)
#ifdef WITH_DEBUG
	#undef cs_debug_mask
	#define cs_debug_mask(x,txt,y...)	cs_log_int(x, 1, NULL, 0, "dvbapi: "txt, ##y)
#endif

#endif // MODULEDVBAPI_H_
#endif // WITH_DVBAPI
