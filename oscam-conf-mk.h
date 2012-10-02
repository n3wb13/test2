#ifndef OSCAM_CONF_MK_H_
#define OSCAM_CONF_MK_H_

extern char *mk_t_caidtab(CAIDTAB *ctab);
extern char *mk_t_caidvaluetab(CAIDVALUETAB *tab);
extern char *mk_t_tuntab(TUNTAB *ttab);
extern char *mk_t_group(uint64_t grp);
extern char *mk_t_ftab(FTAB *ftab);
extern char *mk_t_camd35tcp_port(void);
extern char *mk_t_cccam_port(void);
extern char *mk_t_aeskeys(struct s_reader *rdr);
extern char *mk_t_newcamd_port(void);
extern char *mk_t_aureader(struct s_auth *account);
extern char *mk_t_nano(struct s_reader *rdr, uchar flag);
extern char *mk_t_service( uint64_t sidtabok, uint64_t sidtabno);
extern char *mk_t_logfile(void);
extern char *mk_t_iprange(struct s_ip *range);
extern char *mk_t_ecmwhitelist(struct s_ecmWhitelist *whitelist);
extern char *mk_t_ecmheaderwhitelist(struct s_ecmHeaderwhitelist *headerlist);
extern char *mk_t_cltab(CLASSTAB *clstab);
extern char *mk_t_emmbylen(struct s_reader *rdr);
extern char *mk_t_allowedprotocols(struct s_auth *account);
extern void free_mk_t(char *value);

#endif
