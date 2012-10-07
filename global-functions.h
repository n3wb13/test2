#ifndef GLOBAL_FUNCTIONS_H_
#define GLOBAL_FUNCTIONS_H_

/* ===========================
 *      protocol modules
 * =========================== */
extern int32_t monitor_send_idx(struct s_client *, char *);
extern void module_monitor(struct s_module *);
extern void module_camd35(struct s_module *);
extern void module_camd35_tcp(struct s_module *);
extern void module_camd33(struct s_module *);
extern void module_newcamd(struct s_module *);
extern void module_radegast(struct s_module *);
extern void module_oscam_ser(struct s_module *);
extern void module_cccam(struct s_module *);
extern void module_pandora(struct s_module *);
extern void module_gbox(struct s_module *);
extern void module_constcw(struct s_module *);
extern void module_csp(struct s_module *);
extern void module_dvbapi(struct s_module *);

/* ===========================
 *       card support
 * =========================== */
extern void reader_nagra(struct s_cardsystem *);
extern void reader_irdeto(struct s_cardsystem *);
extern void reader_cryptoworks(struct s_cardsystem *);
extern void reader_viaccess(struct s_cardsystem *);
extern void reader_conax(struct s_cardsystem *);
extern void reader_seca(struct s_cardsystem *);
extern void reader_videoguard1(struct s_cardsystem *);
extern void reader_videoguard2(struct s_cardsystem *);
extern void reader_videoguard12(struct s_cardsystem *);
extern void reader_dre(struct s_cardsystem *);
extern void reader_tongfang(struct s_cardsystem *);
extern void reader_bulcrypt(struct s_cardsystem *);

/* ===========================
 *         cardreaders
 * =========================== */
extern void cardreader_mouse(struct s_cardreader *crdr);
extern void cardreader_smargo(struct s_cardreader *crdr);
extern void cardreader_stapi(struct s_cardreader *crdr);

/* ===========================
 *           oscam
 * =========================== */
extern void cs_exit_oscam(void);
extern void cs_restart_oscam(void);
extern int32_t cs_get_restartmode(void);

int32_t restart_cardreader(struct s_reader *rdr, int32_t restart);

extern int32_t chk_global_whitelist(ECM_REQUEST *er, uint32_t *line);
extern void global_whitelist_read(void);

extern int32_t accept_connection(int32_t i, int32_t j);
extern void start_thread(void * startroutine, char * nameroutine);
extern int32_t add_job(struct s_client *cl, int8_t action, void *ptr, int32_t len);
extern void add_check(struct s_client *client, int8_t action, void *ptr, int32_t size, int32_t ms_delay);
extern int32_t reader_init(struct s_reader *);
extern int reader_reset(struct s_reader * reader);
extern void cs_reload_config(void);
extern int32_t recv_from_udpipe(uchar *);
extern int32_t chk_bcaid(ECM_REQUEST *, CAIDTAB *);
extern void cs_exit(int32_t sig);
extern struct ecm_request_t *check_cwcache(ECM_REQUEST *, struct s_client *);
extern int32_t write_to_pipe(struct s_client *, int32_t, uchar *, int32_t);
extern int32_t read_from_pipe(struct s_client *, uchar **);
extern int32_t write_ecm_answer(struct s_reader *, ECM_REQUEST *, int8_t, uint8_t, uchar *, char *);
extern uint32_t chk_provid(uchar *, uint16_t);
extern uint16_t get_betatunnel_caid_to(uint16_t caid);
extern void convert_to_beta(struct s_client *cl, ECM_REQUEST *er, uint16_t caidto);
extern void get_cw(struct s_client *, ECM_REQUEST *);
extern void do_emm(struct s_client *, EMM_PACKET *);
extern ECM_REQUEST *get_ecmtask(void);
extern int32_t send_dcw(struct s_client *, ECM_REQUEST *);
extern int32_t process_input(uchar *, int32_t, int32_t);
extern void set_signal_handler(int32_t , int32_t , void (*));
extern void cs_waitforcardinit(void);
extern int32_t process_client_pipe(struct s_client *cl, uchar *buf, int32_t l);
extern void update_reader_config(uchar *ptr);
extern void *clientthread_init(void * init);
extern void cleanup_thread(void *var);
extern void kill_thread(struct s_client *cl);
extern void remove_reader_from_active(struct s_reader *rdr);
extern void add_reader_to_active(struct s_reader *rdr);
extern void cs_card_info(void);
extern void cs_debug_level(void);
extern void update_chid(ECM_REQUEST *ecm);
extern void free_ecm(ECM_REQUEST *ecm);

#define debug_ecm(mask, args...) \
	do { \
		if (config_WITH_DEBUG()) { \
			char buf[ECM_FMT_LEN]; \
			format_ecm(er, buf, ECM_FMT_LEN); \
			cs_debug_mask(mask, ##args); \
		} \
	} while(0)

/* ===========================
 *        oscam-config
 * =========================== */
extern int32_t  init_config(void);
extern int32_t  init_free_userdb(struct s_auth *auth);
extern void     account_set_defaults(struct s_auth *auth);
extern void     reader_set_defaults(struct s_reader *rdr);
extern struct s_auth *init_userdb(void);
extern int32_t  init_readerdb(void);
extern void free_reader(struct s_reader *rdr);
extern int32_t  init_sidtab(void);
extern void free_sidtab(struct s_sidtab *sidtab);
extern void init_free_sidtab(void);
extern int32_t init_provid(void);

extern void config_set(char *section, const char *token, char *value);
extern void config_free(void);

extern int32_t  init_srvid(void);
extern int32_t  init_tierid(void);
extern void init_len4caid(void);
extern int32_t csp_ecm_hash(ECM_REQUEST *er);
extern void chk_reader(char *token, char *value, struct s_reader *rdr);

extern void dvbapi_chk_caidtab(char *caidasc, char type);
extern void dvbapi_read_priority(void);

void check_caidtab_fn(const char *token, char *value, void *setting, FILE *f);

extern void cs_accounts_chk(void);
extern void chk_account(const char *token, char *value, struct s_auth *account);
extern void chk_sidtab(char *token, char *value, struct s_sidtab *sidtab);
extern int32_t write_services(void);
extern int32_t write_userdb(void);
extern int32_t write_config(void);
extern int32_t write_server(void);
extern void write_versionfile(void);

/* ===========================
 *         oscam-log
 * =========================== */
extern char *LOG_LIST;
extern int32_t  cs_init_log(void);
extern void cs_reinit_loghist(uint32_t size);
extern int32_t cs_open_logfiles(void);

extern void cs_log_int(uint16_t mask, int8_t lock, const uchar *buf, int32_t n, const char *fmt, ...) __attribute__ ((format (printf, 5, 6)));

#define cs_log(...)          cs_log_int(0, 1, NULL, 0, ##__VA_ARGS__)
#define cs_log_nolock(...)   cs_log_int(0, 0, NULL, 0, ##__VA_ARGS__)
#define cs_dump(buf, n, ...) cs_log_int(0, 1, buf,  n, ##__VA_ARGS__)

#define cs_debug_mask(mask, ...)         do { if (config_WITH_DEBUG()) cs_log_int(mask, 1, NULL, 0, ##__VA_ARGS__); } while(0)
#define cs_debug_mask_nolock(mask, ...)  do { if (config_WITH_DEBUG()) cs_log_int(mask, 0, NULL, 0, ##__VA_ARGS__); } while(0)
#define cs_ddump_mask(mask, buf, n, ...) do { if (config_WITH_DEBUG()) cs_log_int(mask, 1, buf , n, ##__VA_ARGS__); } while(0)

extern void log_emm_request(struct s_reader *);
extern void logCWtoFile(ECM_REQUEST *er, uchar *cw);
extern void cs_log_config(void);
extern void cs_close_log(void);
extern int32_t cs_init_statistics(void);
extern void cs_statistics(struct s_client * client);
extern void cs_disable_log(int8_t disabled);

/* ===========================
 *        oscam-reader
 * =========================== */
extern int32_t reader_cmd2icc(struct s_reader * reader, const uchar *buf, const int32_t l, uchar *response, uint16_t *response_length);
extern int32_t card_write(struct s_reader * reader, const uchar *, const uchar *, uchar *, uint16_t *);
extern int32_t check_sct_len(const unsigned char *data, int32_t off);
extern void rdr_log(struct s_reader * reader, char *,...) __attribute__ ((format (printf, 2, 3)));
extern void rdr_log_sensitive(struct s_reader * reader, char *,...) __attribute__ ((format (printf, 2, 3)));
extern void rdr_debug_mask(struct s_reader * reader, uint16_t mask, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
extern void rdr_debug_mask_sensitive(struct s_reader * reader, uint16_t mask, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
extern void rdr_ddump_mask(struct s_reader * reader, uint16_t mask, const uint8_t * buf, int n, char *fmt, ...) __attribute__ ((format (printf, 5, 6)));
extern void * start_cardreader(void *);
extern void reader_card_info(struct s_reader * reader);
extern int32_t hostResolve(struct s_reader * reader);
extern int32_t network_tcp_connection_open(struct s_reader *);
extern void network_tcp_connection_close(struct s_reader *, char *);
extern void clear_reader_pipe(struct s_reader * reader);
extern void block_connect(struct s_reader *rdr);
extern int32_t is_connect_blocked(struct s_reader *rdr);
void cs_add_entitlement(struct s_reader *rdr, uint16_t caid, uint32_t provid, uint64_t id, uint32_t class, time_t start, time_t end, uint8_t type);
extern void cs_clear_entitlement(struct s_reader *rdr);

extern void reader_do_idle(struct s_reader * reader);
extern int32_t reader_do_emm(struct s_reader * reader, EMM_PACKET *ep);
extern void reader_log_emm(struct s_reader * reader, EMM_PACKET *ep, int32_t i, int32_t rc, struct timeb *tps);
extern void reader_get_ecm(struct s_reader * reader, ECM_REQUEST *er);
extern void casc_check_dcw(struct s_reader * reader, int32_t idx, int32_t rc, uchar *cw);
extern void casc_do_sock_log(struct s_reader * reader);
extern void reader_do_card_info(struct s_reader * reader);

/* ===========================
 *        oscam-simples
 * =========================== */
extern char *remote_txt(void);
extern int32_t comp_timeb(struct timeb *tpa, struct timeb *tpb);
extern time_t cs_timegm(struct tm *tm);
extern struct tm *cs_gmtime_r(const time_t *timep, struct tm *r);
extern char *cs_ctime_r(const time_t *timep, char* buf);
extern void cs_ftime(struct timeb *);
extern void cs_sleepms(uint32_t);
extern void cs_sleepus(uint32_t);
extern void cs_setpriority(int32_t);
extern int32_t check_filled(uchar *value, int32_t length);
extern void clear_sip(struct s_ip **sip);
extern void clear_ptab(struct s_ptab *ptab);
extern void clear_ftab(struct s_ftab *ftab);
extern void clear_caidtab(struct s_caidtab *ctab);
extern void clear_tuntab(struct s_tuntab *ttab);
extern char *get_servicename(struct s_client *cl, uint16_t srvid, uint16_t caid, char *buf);
extern char *get_tiername(uint16_t tierid, uint16_t caid, char *buf);
extern char *get_provider(uint16_t caid, uint32_t provid, char *buf, uint32_t buflen);
void add_provider(uint16_t caid, uint32_t provid, const char *name, const char *sat, const char *lang);
extern uchar fast_rnd(void);
extern void init_rnd(void);
extern int32_t hexserialset(struct s_reader *rdr);
extern char *reader_get_type_desc(struct s_reader * rdr, int32_t extended);
extern void hexserial_to_newcamd(uchar *source, uchar *dest, uint16_t caid);
extern void newcamd_to_hexserial(uchar *source, uchar *dest, uint16_t caid);

extern struct s_reader *get_reader_by_label(char *lbl);

extern void add_ms_to_timespec(struct timespec *timeout, int32_t msec);
extern int32_t add_ms_to_timeb(struct timeb *tb, int32_t ms);

extern int32_t ecmfmt(uint16_t caid, uint32_t prid, uint16_t chid, uint16_t pid, uint16_t srvid, uint16_t l, uint16_t checksum, char *result, size_t size);
extern int32_t format_ecm(ECM_REQUEST *ecm, char *result, size_t size);

/* ===========================
 *       module-newcamd
 * =========================== */
extern const char *newcamd_get_client_name(uint16_t client_id);

/* ===========================
 *       reader-common
 * =========================== */
extern int32_t reader_device_init(struct s_reader * reader);
extern int32_t reader_checkhealth(struct s_reader * reader);
extern void reader_post_process(struct s_reader * reader);
extern int32_t reader_ecm(struct s_reader * reader, ECM_REQUEST *, struct s_ecm_answer *);
extern int32_t reader_emm(struct s_reader * reader, EMM_PACKET *);
extern struct s_cardsystem *get_cardsystem_by_caid(uint16_t caid);
extern void reader_device_close(struct s_reader * reader);
extern int8_t cs_emmlen_is_blocked(struct s_reader *rdr, int16_t len);

#endif
