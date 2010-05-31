#define ADDRLEN      4    // Address length in EMM commands
#define MAX_PROV     16
#define SCT_LEN(sct) (3+((sct[1]&0x0f)<<8)+sct[2])
#define MAX_LEN      256

//Warning: OK = 0 and ERROR = 1 in csctapi !!!
#define SKIPPED 2
#define OK			1
#define ERROR 	0

#include "atr.h"

#define SC_IRDETO 1
#define SC_CRYPTOWORKS 2
#define SC_VIACCESS 3
#define SC_CONAX 4
#define SC_SECA 5
#define SC_VIDEOGUARD2 6
#define SC_DRE 7
#define SC_NAGRA 8

// reader-irdeto
int irdeto_card_init(struct s_reader * reader, ATR atr);
int irdeto_do_ecm(struct s_reader * reader, ECM_REQUEST *);
int irdeto_do_emm(struct s_reader * reader, EMM_PACKET *);
int irdeto_get_emm_type(EMM_PACKET *, struct s_reader *); //s_reader as last argument to remind you that this function picks out OTHER reader than you would expect!
int irdeto_card_info(struct s_reader * reader);
void irdeto_get_emm_filter(struct s_reader * rdr, uchar *filter);

// reader-viaccess
int viaccess_card_init(struct s_reader * reader, ATR atr);
int viaccess_do_ecm(struct s_reader * reader, ECM_REQUEST *);
int viaccess_do_emm(struct s_reader * reader, EMM_PACKET *);
int viaccess_get_emm_type(EMM_PACKET *, struct s_reader *);
int viaccess_card_info(struct s_reader * reader);
void viaccess_get_emm_filter(struct s_reader * rdr, uchar *filter);

// reader-videoguard
int videoguard_card_init(struct s_reader * reader, ATR atr);
int videoguard_do_ecm(struct s_reader * reader, ECM_REQUEST *);
int videoguard_do_emm(struct s_reader * reader, EMM_PACKET *);
int videoguard_get_emm_type(EMM_PACKET *, struct s_reader *);
int videoguard_card_info(struct s_reader * reader);
void videoguard_get_emm_filter(struct s_reader * rdr, uchar *filter);

// reader-cryptoworks
int cryptoworks_card_init(struct s_reader * reader, ATR atr);
int cryptoworks_do_ecm(struct s_reader * reader, ECM_REQUEST *);
int cryptoworks_do_emm(struct s_reader * reader, EMM_PACKET *);
int cryptoworks_get_emm_type(EMM_PACKET *, struct s_reader *);
int cryptoworks_card_info(struct s_reader * reader);
int CheckSctLen(const uchar *, int);
void cryptoworks_get_emm_filter(struct s_reader * rdr, uchar *filter);

// reader-seca
int seca_card_init(struct s_reader * reader, ATR atr);
int seca_do_ecm(struct s_reader * reader, ECM_REQUEST *);
int seca_do_emm(struct s_reader * reader, EMM_PACKET *);
int seca_get_emm_type(EMM_PACKET *, struct s_reader *);
int seca_card_info(struct s_reader * reader);
void seca_get_emm_filter(struct s_reader * rdr, uchar *filter);

// reader nagra2/3
int nagra2_card_init(struct s_reader * reader, ATR atr);
int nagra2_do_ecm(struct s_reader * reader, ECM_REQUEST *er);
int nagra2_card_info(struct s_reader * reader);
int nagra2_do_emm(struct s_reader * reader, EMM_PACKET *);
int nagra2_get_emm_type(EMM_PACKET *, struct s_reader *);
void nagra2_post_process(struct s_reader * reader);
void nagra2_get_emm_filter(struct s_reader * rdr, uchar *filter);
 
// reader-conax
int conax_card_init(struct s_reader * reader, ATR atr);
int conax_do_ecm(struct s_reader * reader, ECM_REQUEST *);
int conax_do_emm(struct s_reader * reader, EMM_PACKET *);
int conax_get_emm_type(EMM_PACKET *, struct s_reader *);
int conax_card_info(struct s_reader * reader);
void conax_get_emm_filter(struct s_reader * rdr, uchar *filter);

// reader-dre
int dre_card_init(struct s_reader * reader, ATR atr);
int dre_do_ecm(struct s_reader * reader, ECM_REQUEST *);
int dre_do_emm(struct s_reader * reader, EMM_PACKET *);
int dre_get_emm_type(EMM_PACKET *, struct s_reader *);
int dre_card_info(void);
void dre_get_emm_filter(struct s_reader * rdr, uchar *filter);

int check_emm_cardsystem(struct s_reader * rdr, EMM_PACKET *ep);

#define write_cmd(cmd, data) \
{ \
        if (card_write(reader, cmd, data, cta_res, &cta_lr)) return ERROR; \
}

#define get_atr \
		unsigned char atr[64]; \
		unsigned int atr_size; \
		memset(atr, 0, sizeof(atr)); \
		ATR_GetRaw(&newatr, atr, &atr_size);

#define get_hist \
		unsigned char hist[64]; \
		unsigned int hist_size; \
		ATR_GetHistoricalBytes(&newatr, hist, &hist_size);

#define def_resp \
		unsigned char cta_res[CTA_RES_LEN]; \
		unsigned short cta_lr;
