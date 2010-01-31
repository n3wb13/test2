/* config.h.  Generated by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

#include "../oscam-config.h"

/* Debug CT-API */
/* #undef DEBUG_CTAPI */
// #define DEBUG_CTAPI 1

/* Debug Integrated Circuit Card */
/* #undef DEBUG_ICC */
// #define DEBUG_ICC 1

/* Debug Interface Device */
/* #undef DEBUG_IFD */
//#define DEBUG_IFD 1

/* Debug IFD Handler */
/* #undef DEBUG_IFDH */
//#define DEBUG_IFDH 1

/* Debug Input/Output */
/* #undef DEBUG_IO */
// #define DEBUG_IO 1

/* Debug Input/Output */
#define DEBUG_USB_IO 1

/* Debug Protocol */
/* #undef DEBUG_PROTOCOL */
//#define DEBUG_PROTOCOL 1

//#define PROTOCOL_T0_ISO 1
//#define PROTOCOL_T1_ISO 1


/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `nanosleep' function. */
#if !defined(OS_AIX) && !defined(OS_SOLARIS) && !defined(OS_OSF5)
#define HAVE_NANOSLEEP 1
#endif

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the <pthread.h> header file. */
//#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `syslog' function. */
#define HAVE_SYSLOG 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Memory size */
/* #undef ICC_SYNC_MEMORY_LENGTH */

/* Memory smartcard type (0,1,2,3) */
/* #undef ICC_SYNC_MEMORY_TYPE */

/* ATR for asynchronous cards is cheked */
#define IFD_TOWITOKO_STRICT_ATR_CHECK 1

/* Enable Linux 2.4.X devfs support */
/* #undef IO_ENABLE_DEVFS */
#define IO_ENABLE_DEVFS 1

/* Enable /dev/pcsc/ links */
/* #undef IO_ENABLE_DEVPCSC */

/* Enable USB support */
//#define IO_ENABLE_USB 1

/* FreeBSD Operating System */
/* #undef OS_AIX */

/* Cywin32 for Windows NT environment */
//#define OS_CYGWIN32 1

/* FreeBSD Operating System */
/* #undef OS_FREEBSD */

/* HPUX Operating System */
/* #undef OS_HPUX */

/* IRIX Operating System */
/* #undef OS_IRIX */

/* Linux Operating System */
//#define OS_LINUX 1

/* FreeBSD Operating System */
/* #undef OS_NETBSD */

/* OpenBSD Operating System */
/* #undef OS_OPENBSD */

/* Other Operating System */
/* #undef OS_OTHER */

/* SCO Operating System */
/* #undef OS_SCO */

/* Sun Operating System */
/* #undef OS_SOLARIS */

/* Name of package */
#define PACKAGE "towitoko"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Transportation of APDUs by T=0 */
/* #undef PROTOCOL_T0_ISO */

/* Timings in ATR are not used in T=0 cards */
/* #undef PROTOCOL_T0_USE_DEFAULT_TIMINGS */

/* Timings in ATR are not used in T=1 cards */
/* #undef PROTOCOL_T1_USE_DEFAULT_TIMINGS */

/* Timings in ATR are not used after PTS */
/* #undef PTS_USE_DEFAULT_TIMINGS */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.0.7"

/* This may be usefull for LinuxThreads */
#define _REENTRANT 1
