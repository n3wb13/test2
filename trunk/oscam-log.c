#include "globals.h"
#include <syslog.h>
#include <stdlib.h>

static FILE *fp=(FILE *)0;
static FILE *fps=(FILE *)0;
static short logStarted = 0;

pthread_mutex_t switching_log;
#ifdef CS_LOGHISTORY
pthread_mutex_t loghistory_lock;
#endif

#ifdef CS_ANTICASC
FILE *fpa=(FILE *)0;
#endif

#define LOG_BUF_SIZE (520+11) // should be aligned with s_client.dump from globals.h

static void switch_log(char* file, FILE **f, int (*pfinit)(void))
{
	if( cfg.max_log_size)	//only 1 thread needs to switch the log; even if anticasc, statistics and normal log are running
					//at the same time, it is ok to have the other logs switching 1 entry later
	{
		struct stat stlog;

		if( stat(file, &stlog)!=0 )
		{
			fprintf(stderr, "stat('%s',..) failed (errno=%d)\n", file, errno);
			return;
		}

		if( stlog.st_size >= cfg.max_log_size*1024 && *f != NULL) {
			int rc;
			char prev_log[strlen(file) + 6];
			snprintf(prev_log, sizeof(prev_log), "%s-prev", file);
			if ( pthread_mutex_trylock(&switching_log) == 0) { //I got the lock so I am the first thread detecting a switchlog is needed
				fprintf(*f, "switch log file\n");
				fflush(*f);
				fclose(*f);
				*f = (FILE *)0;
				rc = rename(file, prev_log);
				if( rc!=0 ) {
					fprintf(stderr, "rename(%s, %s) failed (errno=%d)\n", file, prev_log, errno);
				}
				else
					if( pfinit()){
						fprintf(stderr, "Initialisation of log file failed, continuing without logging thread %8X. Log will be output to stdout!", (unsigned int)pthread_self());
						cfg.logtostdout = 1;
					}
			}
			else //I am not the first to detect a switchlog is needed, so I need to wait for the first thread to complete
				pthread_mutex_lock(&switching_log); //wait on 1st thread
			pthread_mutex_unlock(&switching_log); //release after processing or after waiting
		}
	}
}

void cs_write_log(char *txt)
{
#ifdef CS_ANTICASC
	if( cur_client()->typ == 'a' && fpa ) {
		switch_log(cfg.ac_logfile, &fpa, ac_init_log);
		if (fpa) {
				fputs(txt, fpa);
				fflush(fpa);
		}
	}
	else
#endif
		// filter out entries with leading 's' and forward to statistics
		if(txt[0] == 's') {
			if (fps) {
				switch_log(cfg.usrfile, &fps, cs_init_statistics);
				if (fps) {
						fputs(txt + 1, fps); // remove the leading 's' and write to file
						fflush(fps);
				}
			}
		} else {
			if(!cfg.disablelog){
				if (fp){
					switch_log(cfg.logfile, &fp, cs_open_logfiles);
					if (fp) {
							fputs(txt, fp);
							fflush(fp);
					}		
				}
				if(cfg.logtostdout){
					fputs(txt, stdout);
					fflush(stdout);
				}
			}
		}
}

int cs_open_logfiles()
{
	if (!fp && cfg.logfile != NULL) {	//log to file
		if ((fp = fopen(cfg.logfile, "a+")) <= (FILE *)0) {
			fp = (FILE *)0;
			fprintf(stderr, "couldn't open logfile: %s (errno %d)\n", cfg.logfile, errno);
		} else {
			time_t t;
			char line[80];
			memset(line, '-', sizeof(line));
			line[(sizeof(line)/sizeof(char)) - 1] = '\0';
			time(&t);
			if (!cfg.disablelog)
				fprintf(fp, "\n%s\n>> OSCam <<  cardserver started at %s%s\n", line, ctime(&t), line);
		}
	}
	// according to syslog docu: calling closelog is not necessary and calling openlog multiple times is safe
	// We use openlog to set the default syslog settings so that it's possible to allow switching syslog on and off
	openlog("oscam", LOG_NDELAY, LOG_DAEMON);
	
	cs_log(">> OSCam <<  cardserver started version " CS_VERSION ", build #" CS_SVN_VERSION " (" CS_OSTYPE ")");
	cs_log_config();
	return(fp <= (FILE *)0);
}

int cs_init_log(void)
{
	if(logStarted == 0){
		pthread_mutex_init(&switching_log, NULL);
#ifdef CS_LOGHISTORY
		pthread_mutex_init(&loghistory_lock, NULL);
#endif
	}
	int rc = cs_open_logfiles();
	logStarted = 1;
	return rc;
}

static void get_log_header(int m, char *txt)
{
	if(m)
		sprintf(txt, "%8X %c ",(unsigned int) cur_client()->thread, cur_client()->typ);
	else
		sprintf(txt, "%8X%-3.3s",(unsigned int) cur_client()->thread, "");
}

static void write_to_log(int flag, char *txt)
{
	//flag = -1 is old behaviour, before implementation of debug_nolf (=debug no line feed)
	//
	time_t t;
	struct tm lt;
	char sbuf[16];
	char log_buf[700];

	//  get_log_header(flag, sbuf);
	//  memcpy(txt, sbuf, 11);

#ifdef CS_ANTICASC
	if (cfg.logtosyslog && cur_client()->typ != 'a') // system-logfile
#else
	if (cfg.logtosyslog) // system-logfile
#endif
		syslog(LOG_INFO, txt);

	time(&t);
	localtime_r(&t, &lt);

	switch(flag) {
		case -1:
		snprintf(log_buf, sizeof(log_buf),  "[LOG000]%4d/%02d/%02d %2d:%02d:%02d %s\n",
				lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday,
				lt.tm_hour, lt.tm_min, lt.tm_sec, txt);
		break;
		case 1:
			snprintf(log_buf, sizeof(log_buf), "[LOG000]%4d/%02d/%02d %2d:%02d:%02d            %s",
					lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday,
					lt.tm_hour, lt.tm_min, lt.tm_sec, txt);
			break;
		default:
			snprintf(log_buf, sizeof(log_buf), "[LOG000]%s", txt);
	}

	cs_write_log(log_buf + 8);
	struct s_client *cur_cl = cur_client();
#ifdef CS_LOGHISTORY
	pthread_mutex_lock(&loghistory_lock);
	char *ptr=(char *)(loghist+(loghistidx*CS_LOGHISTSIZE));
	loghistidx=(++loghistidx < CS_MAXLOGHIST)?loghistidx:0;
	pthread_mutex_unlock(&loghistory_lock);

	ptr[0]='\1';    // make username unusable
	ptr[1]='\0';

	if ((cur_cl->typ=='c') || (cur_cl->typ=='m'))
		cs_strncpy(ptr, cur_cl->account?cur_cl->account->usr:"", 63);
	else if ((cur_cl->typ=='p') || (cur_cl->typ=='r'))
		cs_strncpy(ptr, cur_cl->reader->label, 63);
	else
		cs_strncpy(ptr, "server", 63);

	cs_strncpy(ptr+32, log_buf, CS_LOGHISTSIZE-33);

#endif
	struct s_client *cl;
	for (cl=first_client; cl ; cl=cl->next)
	{
	 if ((cl->typ == 'm') && (cl->monlvl>0))
	  if (cl->log) //this variable is only initialized for cl->typ = 'm' 
		{
			if (cl->monlvl<2) {
				if ((cur_cl->typ != 'c') && (cur_cl->typ != 'm'))
					continue;
				if (cur_cl->account && cl->account && strcmp(cur_cl->account->usr, cl->account->usr))
					continue;
			}
			snprintf(sbuf, sizeof(sbuf), "%03d", cl->logcounter);
			cl->logcounter = (cl->logcounter+1) % 1000;
			memcpy(log_buf + 4, sbuf, 3);
			monitor_send_idx(cl, log_buf);
		}
	}
}

void cs_log(const char *fmt,...)
{
	char log_txt[LOG_BUF_SIZE];
	get_log_header(1, log_txt);
	va_list params;
	va_start(params, fmt);
	vsnprintf(log_txt+11, sizeof(log_txt) - 11, fmt, params);
	va_end(params);
	write_to_log(-1, log_txt);
}

void cs_close_log(void)
{
	if (!fp) return;
	fclose(fp);
	fp=(FILE *)0;
}
#ifdef WITH_DEBUG
void cs_debug_mask(unsigned short mask, const char *fmt,...)
{
	char log_txt[LOG_BUF_SIZE];
	if (cs_dblevel & mask)
	{
		get_log_header(1, log_txt);
		va_list params;
		va_start(params, fmt);
		vsnprintf(log_txt+11, sizeof(log_txt) - 11, fmt, params);
		va_end(params);
		write_to_log(-1, log_txt);
	}
}
#endif
void cs_dump(const uchar *buf, int n, char *fmt, ...)
{
	char log_txt[LOG_BUF_SIZE];
	int i;

	if( fmt )
	{
		get_log_header(1, log_txt);
		va_list params;
		va_start(params, fmt);
		vsnprintf(log_txt+11, sizeof(log_txt) - 11, fmt, params);
		va_end(params);
		write_to_log(-1, log_txt);
		//printf("LOG: %s\n", txt); fflush(stdout);
	}

	for( i=0; i<n; i+=16 )
	{
		get_log_header(0, log_txt);
		snprintf(log_txt+11, sizeof(log_txt) - 11, "%s", cs_hexdump(1, buf+i, (n-i>16) ? 16 : n-i));
		write_to_log(-1, log_txt);
	}
}
#ifdef WITH_DEBUG
void cs_ddump_mask(unsigned short mask, const uchar *buf, int n, char *fmt, ...)
{

	char log_txt[LOG_BUF_SIZE];
	int i;
	if ((mask & cs_dblevel) && (fmt))
	{
		get_log_header(1, log_txt);
		va_list params;
		va_start(params, fmt);
		vsnprintf(log_txt+11, sizeof(log_txt) - 11, fmt, params);
		va_end(params);
		write_to_log(-1, log_txt);
		//printf("LOG: %s\n", txt); fflush(stdout);
	}
	if (mask & cs_dblevel)
	{
		for (i=0; i<n; i+=16)
		{
			get_log_header(0, log_txt);
			snprintf(log_txt+11, sizeof(log_txt) - 11, "%s", cs_hexdump(1, buf+i, (n-i>16) ? 16 : n-i));
			write_to_log(-1, log_txt);
		}
	}
}
#endif
int cs_init_statistics(void) 
{
	if ((!fps) && (cfg.usrfile != NULL))
	{
		if ((fps=fopen(cfg.usrfile, "a+"))<=(FILE *)0)
		{
			fps=(FILE *)0;
			cs_log("couldn't open statistics file: %s", cfg.usrfile);
		}
	}
	return(fps<=(FILE *)0);
}

void cs_statistics(struct s_client * client)
{
	if (!cfg.disableuserfile){
		time_t t;
		struct tm lt;
		char buf[LOG_BUF_SIZE];

		float cwps;

		time(&t);
		localtime_r(&t, &lt);
		if (client->cwfound+client->cwnot>0)
		{
			cwps=client->last-client->login;
			cwps/=client->cwfound+client->cwnot;
		}
		else
			cwps=0;

		char *channel ="";
		if(cfg.mon_appendchaninfo)
			channel = get_servicename(client->last_srvid,client->last_caid);

		int lsec;
		if ((client->last_caid == 0xFFFF) && (client->last_srvid == 0xFFFF))
			lsec = client->last - client->login; //client leave calc total duration
		else
			lsec = client->last - client->lastswitch;

		int secs = 0, fullmins = 0, mins = 0, fullhours = 0;

		if((lsec > 0) && (lsec < 1000000)) {
			secs = lsec % 60;
			if (lsec > 60) {
				fullmins = lsec / 60;
				mins = fullmins % 60;
				if(fullmins > 60) {
					fullhours = fullmins / 60;
				}
			}
		}

		/* statistics entry start with 's' to filter it out on other end of pipe
		 * so we can use the same Pipe as Log
		 */
		snprintf(buf, sizeof(buf), "s%02d.%02d.%02d %02d:%02d:%02d %3.1f %s %s %d %d %d %d %d %d %d %ld %ld %02d:%02d:%02d %s %04X:%04X %s\n",
				lt.tm_mday, lt.tm_mon+1, lt.tm_year%100,
				lt.tm_hour, lt.tm_min, lt.tm_sec, cwps,
				client->account->usr,
				cs_inet_ntoa(client->ip),
				client->port,
				client->cwfound,
				client->cwcache,
				client->cwnot,
				client->cwignored,
				client->cwtout,
				client->cwtun,
				client->login,
				client->last,
				fullhours, mins, secs,
				ph[client->ctyp].desc,
				client->last_caid,
				client->last_srvid,
				channel);

		cs_write_log(buf);
	}
}
