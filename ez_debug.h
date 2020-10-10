/**
 *@file ez_debug.h
 *@brief The easiest and simplest debugging interface for applications
 *@date 2016-09-01 (Tan Feng) Created
 *@date 2019-07-16 (Tan Feng) Changed fprintf()s to dprintf()s
 */
#ifndef EZ_DEBUG_H
#define EZ_DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <string.h>

extern pid_t tcgetsid(int fd);
extern int setenv(const char *name, const char *value, int overwrite);
extern int unsetenv(const char *name);
extern int daemon(int nochdir, int noclose);
extern char* __progname;

/* Define the toggler environmental variable  before including me */
#ifndef EZ_DEBUG_ENV
#define EZ_DEBUG_ENV "EZ_DEBUG"
#endif

/* Define the toggler signal before including me */
#ifndef EZ_DEBUG_SIGNAL
#define EZ_DEBUG_SIGNAL SIGURG
#endif
/* Define your own logfile before calling ez_debug(...) */
#ifndef EZ_DEBUG_FILE
#define EZ_DEBUG_FILE "/var/log/ez_debug.log"
#endif


#define ez_debug(...)  \
do {\
    if (!getenv(__progname) && !getenv(EZ_DEBUG_ENV)) break;\
    if (!getenv("EZ_DEBUG_FILE") && (getenv("TERM") && tcgetsid(STDERR_FILENO)>0)) {\
        dprintf(STDERR_FILENO, "[%s]\x1b[1;32m%s#%d\x1b[0;39m\x1b[1;33m@%s\x1b[0;39m: ",__progname,__FILE__,__LINE__,__func__);\
        dprintf(STDERR_FILENO, __VA_ARGS__);\
        dprintf(STDERR_FILENO, "\n");\
    } else { \
        char xhead[256] = {0}, xbody[8192] = {0}; \
        snprintf(xhead, sizeof(xhead)-1, "[%s]%s#%d@%s: ", __progname,__FILE__,__LINE__,__func__); \
        snprintf(xbody, sizeof(xbody)-1,__VA_ARGS__);\
        int fdxxx = open(EZ_DEBUG_FILE, O_CREAT|O_WRONLY|O_APPEND); \
        if (fdxxx < 0) { dprintf(STDERR_FILENO, "Failed to open %s: %s", EZ_DEBUG_FILE, strerror(errno)); break; }\
        dprintf(fdxxx, "%s%s\n", xhead,xbody); \
        close(fdxxx);\
    }\
} while(0)


#define EZ_DEBUG_TOGGLER \
void ez_debug_toggler(int sig); \
void ez_debug_toggler(int sig) \
{ \
    assert(sig); \
    if(getenv(__progname) || getenv(EZ_DEBUG_ENV)) { \
        unsetenv(__progname); \
        unsetenv(EZ_DEBUG_ENV); \
    } else { \
        setenv(__progname, "x", 1); \
    } \
} 

#define EZ_DEBUG_TOGGLE_ON  signal(EZ_DEBUG_SIGNAL, ez_debug_toggler) 


#ifndef ez_trace
#define ez_trace(...) ez_debug(">>>"__VA_ARGS__) 
#endif

#endif  //EZ_DEBUG_H
