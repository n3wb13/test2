//FIXME Not checked on threadsafety yet; after checking please remove this line
#include "globals.h"

int32_t pserver = 0;

int32_t constcw_file_available(void)
{
    FILE *fp;
    
    fp=fopen(cur_client()->reader->device, "r");
    if (!fp) return (0);
    fclose(fp);
    return (1);
}

int32_t constcw_analyse_file(uint16_t c_caid, uint32_t c_prid, uint16_t c_sid, uchar *dcw)
{
    //CAID:PROVIDER:SID:PMT:PID:: XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
    
    FILE *fp;
    char token[4096];
    uint32_t caid, provid, sid, pmt, pid;
    uchar cw[16];

    // FIXME
    c_prid = c_prid;

    fp=fopen(cur_client()->reader->device, "r");
    if (!fp) return (0);
    
    while (fgets(token, sizeof(token), fp))
    {
	if (token[0]=='#') continue;
	
	sscanf(token, "%x:%x:%x:%x:%x::%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x", &caid, &provid, &sid, &pmt, &pid, 
		(uint*) &cw[0], (uint*) &cw[1], (uint*) &cw[2],	(uint*) &cw[3],	
		(uint*) &cw[4], (uint*) &cw[5], (uint*) &cw[6], (uint*) &cw[7], 
		(uint*) &cw[8], (uint*) &cw[9], (uint*) &cw[10], (uint*) &cw[11], 
		(uint*) &cw[12], (uint*) &cw[13], (uint*) &cw[14], (uint*) &cw[15]);

	//cs_log("Line found: %s", token);
	if (c_caid == caid && c_sid == sid)
	{
	    cs_log("Entry found: %04X:%06X:%04X %s", caid, provid, sid, cs_hexdump(1, cw, 16));
	    memcpy(dcw, cw, 16);
	    fclose(fp);
	    return 1;
	}
    }
    
    fclose(fp);
    return 0;
}
//************************************************************************************************************************
//* client/server common functions
//************************************************************************************************************************
static int32_t constcw_recv(struct s_client *client, uchar *buf, int32_t l)
{
    int32_t ret;

    if (!client->udp_fd) return(-9);
    ret = read(client->udp_fd, buf, l);
    if (ret < 1) return(-1);
    client->last = time(NULL);
    return(ret);
}

//************************************************************************************************************************
//*       client functions
//************************************************************************************************************************
int32_t constcw_client_init(struct s_client *client)
{
    int32_t fdp[2];
    
    client->pfd = 0;
    if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fdp))
    {
	cs_log("Socket creation failed (%s)", strerror(errno));
	cs_exit(1);
    }
    client->udp_fd =fdp[0];
    pserver = fdp[1];

    memset((char *) &client->udp_sa, 0, sizeof(client->udp_sa));
    client->udp_sa.sin_family = AF_INET;

    // Oscam has no reader.au in s_reader like ki's mpcs ;)
    // reader[ridx].au = 0;
    // cs_log("local reader: %s (file: %s) constant cw au=0", reader[ridx].label, reader[ridx].device);
    cs_log("local reader: %s (file: %s) constant cw", client->reader->label, client->reader->device);

    client->pfd = client->udp_fd;
    
    if (constcw_file_available())
    {
	client->reader->tcp_connected = 2;
        client->reader->card_status = CARD_INSERTED;
    }

    return(0);
}

static int32_t constcw_send_ecm(struct s_client *client, ECM_REQUEST *er, uchar *msgbuf)
{
    time_t t;
    struct s_reader *rdr = client->reader;

    // FIXME
    msgbuf = msgbuf;

    t = time(NULL);
    // Check if DCW exist in the files
    //cs_log("Searching ConstCW for ECM: %04X:%06X:%04X (%d)", er->caid, er->prid, er->srvid, er->l);
   
    if (constcw_analyse_file(er->caid, er->prid, er->srvid, er->cw)==0)
    {
        er->rc = E_RDR_NOTFOUND;
        er->rcEx = E1_READER<<4 | E2_SID;
	//cs_sleepms(100);
    }

    //cs_sleepms(50);
    write_ecm_answer(rdr, er);
    
    client->last = t;
    rdr->last_g = t;
    return(0);
}

static int32_t constcw_recv_chk(struct s_client *client, uchar *dcw, int32_t *rc, uchar *buf, int32_t n)
{
    // FIXME
    *client = *client;
    dcw = dcw;
    n = n;
    buf = buf;

    *rc = 0;
    return(-1);
}

void module_constcw(struct s_module *ph)
{
  cs_strncpy(ph->desc, "constcw", sizeof(ph->desc));
  ph->type = MOD_NO_CONN;
  ph->multi = 0;
  ph->watchdog = 1;
  ph->recv = constcw_recv;
  
  ph->c_multi = 1;
  ph->c_init = constcw_client_init;
  ph->c_recv_chk = constcw_recv_chk;
  ph->c_send_ecm = constcw_send_ecm;
  ph->num=R_CONSTCW;
}
