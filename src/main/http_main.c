/* ====================================================================
 * Copyright (c) 1995-1998 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

/*
 * httpd.c: simple http daemon for answering WWW file requests
 *
 * 
 * 03-21-93  Rob McCool wrote original code (up to NCSA HTTPd 1.3)
 * 
 * 03-06-95  blong
 *  changed server number for child-alone processes to 0 and changed name
 *   of processes
 *
 * 03-10-95  blong
 *      Added numerous speed hacks proposed by Robert S. Thau (rst@ai.mit.edu) 
 *      including set group before fork, and call gettime before to fork
 *      to set up libraries.
 *
 * 04-14-95  rst / rh
 *      Brandon's code snarfed from NCSA 1.4, but tinkered to work with the
 *      Apache server, and also to have child processes do accept() directly.
 *
 * April-July '95 rst
 *      Extensive rework for Apache.
 */

#define CORE_PRIVATE

#include "httpd.h"
#include "http_main.h"
#include "http_log.h"
#include "http_config.h"	/* for read_config */
#include "http_protocol.h"	/* for read_request */
#include "http_request.h"	/* for process_request */
#include "http_conf_globals.h"
#include "http_core.h"		/* for get_remote_host */
#include "http_vhost.h"
#include "util_script.h"	/* to force util_script.c linking */
#include "scoreboard.h"
#include "multithread.h"
#include <sys/stat.h>
#ifdef USE_SHMGET_SCOREBOARD
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#ifdef SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#endif
#ifdef WIN32
#include "../os/win32/getopt.h"
#elif !defined(BEOS)
#include <netinet/tcp.h>
#endif

#ifdef HAVE_BSTRING_H
#include <bstring.h>		/* for IRIX, FD_SET calls bzero() */
#endif

#ifdef MULTITHREAD
/* special debug stuff -- PCS */

/* Set this non-zero if you are prepared to put up with more than one log entry per second */
#define SEVERELY_VERBOSE	    0

  /* APD1() to APD5() are macros to help us debug. Then can either
   * log to the screen or the error_log file. In release builds, this
   * macros do nothing. In debug builds, they send messages at priority
   * "debug" to the error log file, or if DEBUG_TO_CONSOLE is defined,
   * to the console.
   */

# ifdef _DEBUG
#  ifndef DEBUG_TO_CONSOLE
#   define APD1(a) aplog_error(APLOG_MARK,APLOG_DEBUG|APLOG_NOERRNO,server_conf,a)
#   define APD2(a,b) aplog_error(APLOG_MARK,APLOG_DEBUG|APLOG_NOERRNO,server_conf,a,b)
#   define APD3(a,b,c) aplog_error(APLOG_MARK,APLOG_DEBUG|APLOG_NOERRNO,server_conf,a,b,c)
#   define APD4(a,b,c,d) aplog_error(APLOG_MARK,APLOG_DEBUG|APLOG_NOERRNO,server_conf,a,b,c,d)
#   define APD5(a,b,c,d,e) aplog_error(APLOG_MARK,APLOG_DEBUG|APLOG_NOERRNO,server_conf,a,b,c,d,e)
#  else
#   define APD1(a) printf("%s\n",a)
#   define APD2(a,b) do { printf(a,b);putchar('\n'); } while(0);
#   define APD3(a,b,c) do { printf(a,b,c);putchar('\n'); } while(0);
#   define APD4(a,b,c,d) do { printf(a,b,c,d);putchar('\n'); } while(0);
#   define APD5(a,b,c,d,e) do { printf(a,b,c,d,e);putchar('\n'); } while(0);
#  endif
# else /* !_DEBUG */
#  define APD1(a) 
#  define APD2(a,b) 
#  define APD3(a,b,c) 
#  define APD4(a,b,c,d) 
#  define APD5(a,b,c,d,e) 
# endif /* _DEBUG */
#endif /* MULTITHREAD */

/* This next function is never used. It is here to ensure that if we
 * make all the modules into shared libraries that core httpd still
 * includes the full Apache API. Without this function the objects in
 * main/util_script.c would not be linked into a minimal httpd.
 */
void force_library_loading(void) {
    add_cgi_vars(NULL);
}

#include "explain.h"

#if !defined(max)
#define max(a,b)        (a > b ? a : b)
#endif

#ifdef WIN32
#include "../os/win32/service.h"
#endif


#ifdef MINT
long _stksize = 32768;
#endif

#ifdef __EMX__
    /* Add MMAP style functionality to OS/2 */
#ifdef USE_MMAP_SCOREBOARD
#define INCL_DOSMEMMGR
#include <os2.h>
#include <umalloc.h>
#include <stdio.h>
caddr_t create_shared_heap(const char *, size_t);
caddr_t get_shared_heap(const char *);
#endif
#endif

DEF_Explain

/* Defining GPROF when compiling uses the moncontrol() function to
 * disable gprof profiling in the parent, and enable it only for
 * request processing in children (or in one_process mode).  It's
 * absolutely required to get useful gprof results under linux
 * because the profile itimers and such are disabled across a
 * fork().  It's probably useful elsewhere as well.
 */
#ifdef GPROF
extern void moncontrol(int);
#define MONCONTROL(x) moncontrol(x)
#else
#define MONCONTROL(x)
#endif

#ifndef MULTITHREAD
/* this just need to be anything non-NULL */
void *dummy_mutex = &dummy_mutex;
#endif

/*
 * Actual definitions of config globals... here because this is
 * for the most part the only code that acts on 'em.  (Hmmm... mod_main.c?)
 */

int standalone;
uid_t user_id;
char *user_name;
gid_t group_id;
#ifdef MULTIPLE_GROUPS
gid_t group_id_list[NGROUPS_MAX];
#endif
int max_requests_per_child;
int threads_per_child;
int excess_requests_per_child;
char *pid_fname;
char *scoreboard_fname;
char *lock_fname;
char *server_argv0;
struct in_addr bind_address;
int daemons_to_start;
int daemons_min_free;
int daemons_max_free;
int daemons_limit;
time_t restart_time;
int suexec_enabled = 0;
int listenbacklog;

/*
 * The max child slot ever assigned, preserved across restarts.  Necessary
 * to deal with MaxClients changes across SIGUSR1 restarts.  We use this
 * value to optimize routines that have to scan the entire scoreboard.
 */
static int max_daemons_limit = -1;

/*
 * During config time, listeners is treated as a NULL-terminated list.
 * child_main previously would start at the beginning of the list each time
 * through the loop, so a socket early on in the list could easily starve out
 * sockets later on in the list.  The solution is to start at the listener
 * after the last one processed.  But to do that fast/easily in child_main it's
 * way more convenient for listeners to be a ring that loops back on itself.
 * The routine setup_listeners() is called after config time to both open up
 * the sockets and to turn the NULL-terminated list into a ring that loops back
 * on itself.
 *
 * head_listener is used by each child to keep track of what they consider
 * to be the "start" of the ring.  It is also set by make_child to ensure
 * that new children also don't starve any sockets.
 *
 * Note that listeners != NULL is ensured by read_config().
 */
listen_rec *listeners;
static listen_rec *head_listener;

char server_root[MAX_STRING_LEN];
char server_confname[MAX_STRING_LEN];
char coredump_dir[MAX_STRING_LEN];

array_header *server_pre_read_config;
array_header *server_post_read_config;

/* *Non*-shared http_main globals... */

server_rec *server_conf;
JMP_BUF APACHE_TLS jmpbuffer;
int sd;
static fd_set listenfds;
static int listenmaxfd;
pid_t pgrp;

/* one_process --- debugging mode variable; can be set from the command line
 * with the -X flag.  If set, this gets you the child_main loop running
 * in the process which originally started up (no detach, no make_child),
 * which is a pretty nice debugging environment.  (You'll get a SIGHUP
 * early in standalone_main; just continue through.  This is the server
 * trying to kill off any child processes which it might have lying
 * around --- Apache doesn't keep track of their pids, it just sends
 * SIGHUP to the process group, ignoring it in the root process.
 * Continue through and you'll be fine.).
 */

int one_process = 0;

/* set if timeouts are to be handled by the children and not by the parent.
 * i.e. child_timeouts = !standalone || one_process.
 */
static int child_timeouts;

#ifdef DEBUG_SIGSTOP
int raise_sigstop_flags;
#endif

#ifndef NO_OTHER_CHILD
/* used to maintain list of children which aren't part of the scoreboard */
typedef struct other_child_rec other_child_rec;
struct other_child_rec {
    other_child_rec *next;
    int pid;
    void (*maintenance) (int, void *, int);
    void *data;
    int write_fd;
};
static other_child_rec *other_children;
#endif

static pool *pconf;		/* Pool for config stuff */
static pool *ptrans;		/* Pool for per-transaction stuff */
static pool *pchild;		/* Pool for httpd child stuff */
static pool *pcommands;	/* Pool for -C and -c switches */

int APACHE_TLS my_pid;		/* it seems silly to call getpid all the time */
#ifndef MULTITHREAD
static int my_child_num;
#endif

scoreboard *scoreboard_image = NULL;

static APACHE_TLS int volatile exit_after_unblock = 0;

/* a clean exit from a child with proper cleanup */
static void __attribute__((noreturn)) clean_child_exit(int code)
{
    if (pchild) {
	child_exit_modules(pchild, server_conf);
	destroy_pool(pchild);
    }
    exit(code);
}

#if defined(USE_FCNTL_SERIALIZED_ACCEPT) || defined(USE_FLOCK_SERIALIZED_ACCEPT)
static void expand_lock_fname(pool *p)
{
    char buf[20];

    ap_snprintf(buf, sizeof(buf), ".%u", getpid());
    lock_fname = pstrcat(p, server_root_relative(p, lock_fname), buf, NULL);
}
#endif

#if defined (USE_USLOCK_SERIALIZED_ACCEPT)

#include <ulocks.h>

static ulock_t uslock = NULL;

#define accept_mutex_child_init(x)

static void accept_mutex_init(pool *p)
{
    ptrdiff_t old;
    usptr_t *us;


    /* default is 8, allocate enough for all the children plus the parent */
    if ((old = usconfig(CONF_INITUSERS, HARD_SERVER_LIMIT + 1)) == -1) {
	perror("usconfig(CONF_INITUSERS)");
	exit(-1);
    }

    if ((old = usconfig(CONF_LOCKTYPE, US_NODEBUG)) == -1) {
	perror("usconfig(CONF_LOCKTYPE)");
	exit(-1);
    }
    if ((old = usconfig(CONF_ARENATYPE, US_SHAREDONLY)) == -1) {
	perror("usconfig(CONF_ARENATYPE)");
	exit(-1);
    }
    if ((us = usinit("/dev/zero")) == NULL) {
	perror("usinit");
	exit(-1);
    }

    if ((uslock = usnewlock(us)) == NULL) {
	perror("usnewlock");
	exit(-1);
    }
}

static void accept_mutex_on()
{
    switch (ussetlock(uslock)) {
    case 1:
	/* got lock */
	break;
    case 0:
	fprintf(stderr, "didn't get lock\n");
	exit(-1);
    case -1:
	perror("ussetlock");
	exit(-1);
    }
}

static void accept_mutex_off()
{
    if (usunsetlock(uslock) == -1) {
	perror("usunsetlock");
	exit(-1);
    }
}

#elif defined (USE_PTHREAD_SERIALIZED_ACCEPT)

/* This code probably only works on Solaris ... but it works really fast
 * on Solaris.  Note that pthread mutexes are *NOT* released when a task
 * dies ... the task has to free it itself.  So we block signals and
 * try to be nice about releasing the mutex.
 */

#include <pthread.h>

static pthread_mutex_t *accept_mutex = (void *)(caddr_t) -1;
static int have_accept_mutex;
static sigset_t accept_block_mask;
static sigset_t accept_previous_mask;

static void accept_mutex_child_cleanup(void *foo)
{
    if (accept_mutex != (void *)(caddr_t)-1
	&& have_accept_mutex) {
	pthread_mutex_unlock(accept_mutex);
    }
}

static void accept_mutex_child_init(pool *p)
{
    register_cleanup(p, NULL, accept_mutex_child_cleanup, null_cleanup);
}

static void accept_mutex_cleanup(void *foo)
{
    if (accept_mutex != (void *)(caddr_t)-1
	&& munmap((caddr_t) accept_mutex, sizeof(*accept_mutex))) {
	perror("munmap");
    }
    accept_mutex = (void *)(caddr_t)-1;
}

static void accept_mutex_init(pool *p)
{
    pthread_mutexattr_t mattr;
    int fd;

    fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
	perror("open(/dev/zero)");
	exit(1);
    }
    accept_mutex = (pthread_mutex_t *) mmap((caddr_t) 0, sizeof(*accept_mutex),
				 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (accept_mutex == (void *) (caddr_t) - 1) {
	perror("mmap");
	exit(1);
    }
    close(fd);
    if ((errno = pthread_mutexattr_init(&mattr))) {
	perror("pthread_mutexattr_init");
	exit(1);
    }
    if ((errno = pthread_mutexattr_setpshared(&mattr,
						PTHREAD_PROCESS_SHARED))) {
	perror("pthread_mutexattr_setpshared");
	exit(1);
    }
    if ((errno = pthread_mutex_init(accept_mutex, &mattr))) {
	perror("pthread_mutex_init");
	exit(1);
    }
    sigfillset(&accept_block_mask);
    sigdelset(&accept_block_mask, SIGHUP);
    sigdelset(&accept_block_mask, SIGTERM);
    sigdelset(&accept_block_mask, SIGUSR1);
    register_cleanup(p, NULL, accept_mutex_cleanup, null_cleanup);
}

static void accept_mutex_on()
{
    int err;

    if (sigprocmask(SIG_BLOCK, &accept_block_mask, &accept_previous_mask)) {
	perror("sigprocmask(SIG_BLOCK)");
	clean_child_exit(1);
    }
    if ((err = pthread_mutex_lock(accept_mutex))) {
	errno = err;
	perror("pthread_mutex_lock");
	clean_child_exit(1);
    }
    have_accept_mutex = 1;
}

static void accept_mutex_off()
{
    int err;

    if ((err = pthread_mutex_unlock(accept_mutex))) {
	errno = err;
	perror("pthread_mutex_unlock");
	clean_child_exit(1);
    }
    /* There is a slight race condition right here... if we were to die right
     * now, we'd do another pthread_mutex_unlock.  Now, doing that would let
     * another process into the mutex.  pthread mutexes are designed to be
     * fast, as such they don't have protection for things like testing if the
     * thread owning a mutex is actually unlocking it (or even any way of
     * testing who owns the mutex).
     *
     * If we were to unset have_accept_mutex prior to releasing the mutex
     * then the race could result in the server unable to serve hits.  Doing
     * it this way means that the server can continue, but an additional
     * child might be in the critical section ... at least it's still serving
     * hits.
     */
    have_accept_mutex = 0;
    if (sigprocmask(SIG_SETMASK, &accept_previous_mask, NULL)) {
	perror("sigprocmask(SIG_SETMASK)");
	clean_child_exit(1);
    }
}

#elif defined (USE_SYSVSEM_SERIALIZED_ACCEPT)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#ifdef NEED_UNION_SEMUN
/* it makes no sense, but this isn't defined on solaris */
union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

#endif

static int sem_id = -1;
static struct sembuf op_on;
static struct sembuf op_off;

/* We get a random semaphore ... the lame sysv semaphore interface
 * means we have to be sure to clean this up or else we'll leak
 * semaphores.
 */
static void accept_mutex_cleanup(void *foo)
{
    union semun ick;

    if (sem_id < 0)
	return;
    /* this is ignored anyhow */
    ick.val = 0;
    semctl(sem_id, 0, IPC_RMID, ick);
}

#define accept_mutex_child_init(x)

static void accept_mutex_init(pool *p)
{
    union semun ick;
    struct semid_ds buf;

    /* acquire the semaphore */
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (sem_id < 0) {
	perror("semget");
	exit(1);
    }
    ick.val = 1;
    if (semctl(sem_id, 0, SETVAL, ick) < 0) {
	perror("semctl(SETVAL)");
	exit(1);
    }
    if (!getuid()) {
	/* restrict it to use only by the appropriate user_id ... not that this
	 * stops CGIs from acquiring it and dinking around with it.
	 */
	buf.sem_perm.uid = user_id;
	buf.sem_perm.gid = group_id;
	buf.sem_perm.mode = 0600;
	ick.buf = &buf;
	if (semctl(sem_id, 0, IPC_SET, ick) < 0) {
	    perror("semctl(IPC_SET)");
	    exit(1);
	}
    }
    register_cleanup(p, NULL, accept_mutex_cleanup, null_cleanup);

    /* pre-initialize these */
    op_on.sem_num = 0;
    op_on.sem_op = -1;
    op_on.sem_flg = SEM_UNDO;
    op_off.sem_num = 0;
    op_off.sem_op = 1;
    op_off.sem_flg = SEM_UNDO;
}

static void accept_mutex_on()
{
    if (semop(sem_id, &op_on, 1) < 0) {
	perror("accept_mutex_on");
	clean_child_exit(1);
    }
}

static void accept_mutex_off()
{
    if (semop(sem_id, &op_off, 1) < 0) {
	perror("accept_mutex_off");
	clean_child_exit(1);
    }
}

#elif defined(USE_FCNTL_SERIALIZED_ACCEPT)
static struct flock lock_it;
static struct flock unlock_it;

static int lock_fd = -1;

#define accept_mutex_child_init(x)

/*
 * Initialize mutex lock.
 * Must be safe to call this on a restart.
 */
static void accept_mutex_init(pool *p)
{

    lock_it.l_whence = SEEK_SET;	/* from current point */
    lock_it.l_start = 0;		/* -"- */
    lock_it.l_len = 0;			/* until end of file */
    lock_it.l_type = F_WRLCK;		/* set exclusive/write lock */
    lock_it.l_pid = 0;			/* pid not actually interesting */
    unlock_it.l_whence = SEEK_SET;	/* from current point */
    unlock_it.l_start = 0;		/* -"- */
    unlock_it.l_len = 0;		/* until end of file */
    unlock_it.l_type = F_UNLCK;		/* set exclusive/write lock */
    unlock_it.l_pid = 0;		/* pid not actually interesting */

    expand_lock_fname(p);
    lock_fd = popenf(p, lock_fname, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (lock_fd == -1) {
	perror("open");
	fprintf(stderr, "Cannot open lock file: %s\n", lock_fname);
	exit(1);
    }
    unlink(lock_fname);
}

static void accept_mutex_on(void)
{
    int ret;

    while ((ret = fcntl(lock_fd, F_SETLKW, &lock_it)) < 0 && errno == EINTR) {
	/* nop */
    }

    if (ret < 0) {
	aplog_error(APLOG_MARK, APLOG_EMERG, server_conf,
		    "fcntl: F_SETLKW: Error getting accept lock, exiting!  "
		    "Perhaps you need to use the LockFile directive to place "
		    "your lock file on a local disk!");
	clean_child_exit(1);
    }
}

static void accept_mutex_off(void)
{
    int ret;

    while ((ret = fcntl(lock_fd, F_SETLKW, &unlock_it)) < 0 && errno == EINTR) {
	/* nop */
    }
    if (ret < 0) {
	aplog_error(APLOG_MARK, APLOG_EMERG, server_conf,
		    "fcntl: F_SETLKW: Error freeing accept lock, exiting!  "
		    "Perhaps you need to use the LockFile directive to place "
		    "your lock file on a local disk!");
	clean_child_exit(1);
    }
}

#elif defined(USE_FLOCK_SERIALIZED_ACCEPT)

static int lock_fd = -1;

static void accept_mutex_cleanup(void *foo)
{
    unlink(lock_fname);
}

/*
 * Initialize mutex lock.
 * Done by each child at it's birth
 */
static void accept_mutex_child_init(pool *p)
{

    lock_fd = popenf(p, lock_fname, O_WRONLY, 0600);
    if (lock_fd == -1) {
	aplog_error(APLOG_MARK, APLOG_EMERG, server_conf,
		    "Child cannot open lock file: %s\n", lock_fname);
	clean_child_exit(1);
    }
}

/*
 * Initialize mutex lock.
 * Must be safe to call this on a restart.
 */
static void accept_mutex_init(pool *p)
{
    expand_lock_fname(p);
    unlink(lock_fname);
    lock_fd = popenf(p, lock_fname, O_CREAT | O_WRONLY | O_EXCL, 0600);
    if (lock_fd == -1) {
	aplog_error(APLOG_MARK, APLOG_EMERG, server_conf,
		    "Parent cannot open lock file: %s\n", lock_fname);
	exit(1);
    }
    register_cleanup(p, NULL, accept_mutex_cleanup, null_cleanup);
}

static void accept_mutex_on(void)
{
    int ret;

    while ((ret = flock(lock_fd, LOCK_EX)) < 0 && errno == EINTR)
	continue;

    if (ret < 0) {
	aplog_error(APLOG_MARK, APLOG_EMERG, server_conf,
		    "flock: LOCK_EX: Error getting accept lock. Exiting!");
	clean_child_exit(1);
    }
}

static void accept_mutex_off(void)
{
    if (flock(lock_fd, LOCK_UN) < 0) {
	aplog_error(APLOG_MARK, APLOG_EMERG, server_conf,
		    "flock: LOCK_UN: Error freeing accept lock. Exiting!");
	clean_child_exit(1);
    }
}

#else
/* Default --- no serialization.  Other methods *could* go here,
 * as #elifs...
 */
#if !defined(MULTITHREAD)
/* Multithreaded systems don't complete between processes for
 * the sockets. */
#define NO_SERIALIZED_ACCEPT
#define accept_mutex_child_init(x)
#define accept_mutex_init(x)
#define accept_mutex_on()
#define accept_mutex_off()
#endif
#endif

/* On some architectures it's safe to do unserialized accept()s in the single
 * Listen case.  But it's never safe to do it in the case where there's
 * multiple Listen statements.  Define SINGLE_LISTEN_UNSERIALIZED_ACCEPT
 * when it's safe in the single Listen case.
 */
#ifdef SINGLE_LISTEN_UNSERIALIZED_ACCEPT
#define SAFE_ACCEPT(stmt) do {if(listeners->next != listeners) {stmt;}} while(0)
#else
#define SAFE_ACCEPT(stmt) do {stmt;} while(0)
#endif

void usage(char *bin)
{
    fprintf(stderr, "Usage: %s [-d directory] [-f file] [-C \"directive\"] [-c \"directive\"] [-v] [-h] [-l]\n", bin);
    fprintf(stderr, "-d directory : specify an alternate initial ServerRoot\n");
    fprintf(stderr, "-f file : specify an alternate ServerConfigFile\n");
    fprintf(stderr, "-C \"directive\" : process directive before reading config files\n");
    fprintf(stderr, "-c \"directive\" : process directive after reading config files\n");
    fprintf(stderr, "-v : show version number\n");
    fprintf(stderr, "-V : show compile settings\n");
    fprintf(stderr, "-h : list directives\n");
    fprintf(stderr, "-l : list modules\n");
    exit(1);
}

/*****************************************************************
 *
 * Timeout handling.  DISTINCTLY not thread-safe, but all this stuff
 * has to change for threads anyway.  Note that this code allows only
 * one timeout in progress at a time...
 */

static APACHE_TLS conn_rec *volatile current_conn;
static APACHE_TLS request_rec *volatile timeout_req;
static APACHE_TLS const char *volatile timeout_name = NULL;
static APACHE_TLS int volatile alarms_blocked = 0;
static APACHE_TLS int volatile alarm_pending = 0;

void timeout(int sig)
{				/* Also called on SIGPIPE */
    char errstr[MAX_STRING_LEN];
    void *dirconf;

    signal(SIGPIPE, SIG_IGN);	/* Block SIGPIPE */
    if (alarms_blocked) {
	alarm_pending = 1;
	return;
    }
    if (exit_after_unblock) {
	clean_child_exit(0);
    }

    if (!current_conn) {
	ap_longjmp(jmpbuffer, 1);
    }

    if (timeout_req != NULL)
	dirconf = timeout_req->per_dir_config;
    else
	dirconf = current_conn->server->lookup_defaults;
    if (sig == SIGPIPE) {
	ap_snprintf(errstr, sizeof(errstr),
		    "%s client stopped connection before %s completed",
		    get_remote_host(current_conn, dirconf, REMOTE_NAME),
		    timeout_name ? timeout_name : "request");
    }
    else {
	ap_snprintf(errstr, sizeof(errstr), "%s timed out for %s",
		    timeout_name ? timeout_name : "request",
		    get_remote_host(current_conn, dirconf, REMOTE_NAME));
    }

    if (!current_conn->keptalive)
	aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_WARNING,
		    current_conn->server, errstr);

    if (timeout_req) {
	/* Someone has asked for this transaction to just be aborted
	 * if it times out...
	 */

	request_rec *log_req = timeout_req;

	while (log_req->main || log_req->prev) {
	    /* Get back to original request... */
	    if (log_req->main)
		log_req = log_req->main;
	    else
		log_req = log_req->prev;
	}

	if (!current_conn->keptalive)
	    log_transaction(log_req);

	bsetflag(timeout_req->connection->client, B_EOUT, 1);
	bclose(timeout_req->connection->client);

	if (!standalone)
	    exit(0);

	ap_longjmp(jmpbuffer, 1);
    }
    else {			/* abort the connection */
	bsetflag(current_conn->client, B_EOUT, 1);
	current_conn->aborted = 1;
    }
}

/*
 * These two called from alloc.c to protect its critical sections...
 * Note that they can nest (as when destroying the sub_pools of a pool
 * which is itself being cleared); we have to support that here.
 */

API_EXPORT(void) block_alarms()
{
    ++alarms_blocked;
}

API_EXPORT(void) unblock_alarms()
{
    --alarms_blocked;
    if (alarms_blocked == 0) {
	if (exit_after_unblock) {
	    /* We have a couple race conditions to deal with here, we can't
	     * allow a timeout that comes in this small interval to allow
	     * the child to jump back to the main loop.  Instead we block
	     * alarms again, and then note that exit_after_unblock is
	     * being dealt with.  We choose this way to solve this so that
	     * the common path through unblock_alarms() is really short.
	     */
	    ++alarms_blocked;
	    exit_after_unblock = 0;
	    clean_child_exit(0);
	}
	if (alarm_pending) {
	    alarm_pending = 0;
	    timeout(0);
	}
    }
}


static APACHE_TLS void (*volatile alarm_fn) (int) = NULL;
#ifdef WIN32
static APACHE_TLS unsigned int alarm_expiry_time = 0;
#endif /* WIN32 */

#ifndef WIN32
static void alrm_handler(int sig)
{
    if (alarm_fn) {
	(*alarm_fn) (sig);
    }
}
#endif

unsigned int set_callback_and_alarm(void (*fn) (int), int x)
{
    unsigned int old;

#ifdef WIN32
    old = alarm_expiry_time;
    if (old)
	old -= time(0);
    if (x == 0) {
	alarm_fn = NULL;
	alarm_expiry_time = 0;
    }
    else {
	alarm_fn = fn;
	alarm_expiry_time = time(NULL) + x;
    }
#else
    if (x) {
	alarm_fn = fn;
    }
#ifndef OPTIMIZE_TIMEOUTS
    old = alarm(x);
#else
    if (child_timeouts) {
	old = alarm(x);
    }
    else {
	/* Just note the timeout in our scoreboard, no need to call the system.
	 * We also note that the virtual time has gone forward.
	 */
	old = scoreboard_image->servers[my_child_num].timeout_len;
	scoreboard_image->servers[my_child_num].timeout_len = x;
	++scoreboard_image->servers[my_child_num].cur_vtime;
    }
#endif
#endif
    return (old);
}


int check_alarm(void)
{
#ifdef WIN32
    if (alarm_expiry_time) {
	unsigned int t;

	t = time(NULL);
	if (t >= alarm_expiry_time) {
	    alarm_expiry_time = 0;
	    (*alarm_fn) (0);
	    return (-1);
	}
	else {
	    return (alarm_expiry_time - t);
	}
    }
    else
	return (0);
#else
    return (0);
#endif /* WIN32 */
}



/* reset_timeout (request_rec *) resets the timeout in effect,
 * as long as it hasn't expired already.
 */

API_EXPORT(void) reset_timeout(request_rec *r)
{
    int i;

    if (timeout_name) {		/* timeout has been set */
	i = set_callback_and_alarm(alarm_fn, r->server->timeout);
	if (i == 0)		/* timeout already expired, so set it back to 0 */
	    set_callback_and_alarm(alarm_fn, 0);
    }
}




void keepalive_timeout(char *name, request_rec *r)
{
    unsigned int to;

    timeout_req = r;
    timeout_name = name;

    if (r->connection->keptalive)
	to = r->server->keep_alive_timeout;
    else
	to = r->server->timeout;
    set_callback_and_alarm(timeout, to);

}

API_EXPORT(void) hard_timeout(char *name, request_rec *r)
{
    timeout_req = r;
    timeout_name = name;

    set_callback_and_alarm(timeout, r->server->timeout);

}

API_EXPORT(void) soft_timeout(char *name, request_rec *r)
{
    timeout_name = name;

    set_callback_and_alarm(timeout, r->server->timeout);

}

API_EXPORT(void) kill_timeout(request_rec *dummy)
{
    set_callback_and_alarm(NULL, 0);
    timeout_req = NULL;
    timeout_name = NULL;
}


/*
 * More machine-dependent networking gooo... on some systems,
 * you've got to be *really* sure that all the packets are acknowledged
 * before closing the connection, since the client will not be able
 * to see the last response if their TCP buffer is flushed by a RST
 * packet from us, which is what the server's TCP stack will send
 * if it receives any request data after closing the connection.
 *
 * In an ideal world, this function would be accomplished by simply
 * setting the socket option SO_LINGER and handling it within the
 * server's TCP stack while the process continues on to the next request.
 * Unfortunately, it seems that most (if not all) operating systems
 * block the server process on close() when SO_LINGER is used.
 * For those that don't, see USE_SO_LINGER below.  For the rest,
 * we have created a home-brew lingering_close.
 *
 * Many operating systems tend to block, puke, or otherwise mishandle
 * calls to shutdown only half of the connection.  You should define
 * NO_LINGCLOSE in conf.h if such is the case for your system.
 */
#ifndef MAX_SECS_TO_LINGER
#define MAX_SECS_TO_LINGER 30
#endif

#ifdef USE_SO_LINGER
#define NO_LINGCLOSE		/* The two lingering options are exclusive */

static void sock_enable_linger(int s)
{
    struct linger li;

    li.l_onoff = 1;
    li.l_linger = MAX_SECS_TO_LINGER;

    if (setsockopt(s, SOL_SOCKET, SO_LINGER,
		   (char *) &li, sizeof(struct linger)) < 0) {
	aplog_error(APLOG_MARK, APLOG_WARNING, server_conf,
	            "setsockopt: (SO_LINGER)");
	/* not a fatal error */
    }
}

#else
#define sock_enable_linger(s)	/* NOOP */
#endif /* USE_SO_LINGER */

#ifndef NO_LINGCLOSE

/* Special version of timeout for lingering_close */

static void lingerout(int sig)
{
    if (alarms_blocked) {
	alarm_pending = 1;
	return;
    }

    if (!current_conn) {
	ap_longjmp(jmpbuffer, 1);
    }
    bsetflag(current_conn->client, B_EOUT, 1);
    current_conn->aborted = 1;
}

static void linger_timeout(void)
{
    timeout_name = "lingering close";

    set_callback_and_alarm(lingerout, MAX_SECS_TO_LINGER);
}

/* Since many clients will abort a connection instead of closing it,
 * attempting to log an error message from this routine will only
 * confuse the webmaster.  There doesn't seem to be any portable way to
 * distinguish between a dropped connection and something that might be
 * worth logging.
 */
static void lingering_close(request_rec *r)
{
    char dummybuf[2048];
    struct timeval tv;
    fd_set lfds, fds_read, fds_err;
    int select_rv = 0, read_rv = 0;
    int lsd;

    /* Prevent a slow-drip client from holding us here indefinitely */

    linger_timeout();

    /* Send any leftover data to the client, but never try to again */

    if (bflush(r->connection->client) == -1) {
	kill_timeout(r);
	bclose(r->connection->client);
	return;
    }
    bsetflag(r->connection->client, B_EOUT, 1);

    /* Close our half of the connection --- send the client a FIN */

    lsd = r->connection->client->fd;

    if ((shutdown(lsd, 1) != 0) || r->connection->aborted) {
	kill_timeout(r);
	bclose(r->connection->client);
	return;
    }

    /* Set up to wait for readable data on socket... */

    FD_ZERO(&lfds);
    FD_SET(lsd, &lfds);

    /* Wait for readable data or error condition on socket;
     * slurp up any data that arrives...  We exit when we go for 
     * an interval of tv length without getting any more data, get an
     * error from select(), get an exception on lsd, get an error or EOF
     * on a read, or the timer expires.
     */

    do {
	/* We use a 2 second timeout because current (Feb 97) browsers
	 * fail to close a connection after the server closes it.  Thus,
	 * to avoid keeping the child busy, we are only lingering long enough
	 * for a client that is actively sending data on a connection.
	 * This should be sufficient unless the connection is massively
	 * losing packets, in which case we might have missed the RST anyway.
	 * These parameters are reset on each pass, since they might be
	 * changed by select.
	 */
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	read_rv = 0;
	fds_read = lfds;
	fds_err = lfds;

	select_rv = ap_select(lsd + 1, &fds_read, NULL, &fds_err, &tv);
    } while ((select_rv > 0) &&	/* Something to see on socket    */
	     !FD_ISSET(lsd, &fds_err) &&	/* that isn't an error condition */
	     FD_ISSET(lsd, &fds_read) &&	/* and is worth trying to read   */
	     ((read_rv = read(lsd, dummybuf, sizeof dummybuf)) > 0));

    /* Should now have seen final ack.  Safe to finally kill socket */

    bclose(r->connection->client);

    kill_timeout(r);
}
#endif /* ndef NO_LINGCLOSE */

/*****************************************************************
 * dealing with other children
 */

#ifndef NO_OTHER_CHILD
void register_other_child(int pid,
		       void (*maintenance) (int reason, void *, int status),
			  void *data, int write_fd)
{
    other_child_rec *ocr;

    ocr = palloc(pconf, sizeof(*ocr));
    ocr->pid = pid;
    ocr->maintenance = maintenance;
    ocr->data = data;
    ocr->write_fd = write_fd;
    ocr->next = other_children;
    other_children = ocr;
}

/* note that since this can be called by a maintenance function while we're
 * scanning the other_children list, all scanners should protect themself
 * by loading ocr->next before calling any maintenance function.
 */
void unregister_other_child(void *data)
{
    other_child_rec **pocr, *nocr;

    for (pocr = &other_children; *pocr; pocr = &(*pocr)->next) {
	if ((*pocr)->data == data) {
	    nocr = (*pocr)->next;
	    (*(*pocr)->maintenance) (OC_REASON_UNREGISTER, (*pocr)->data, -1);
	    *pocr = nocr;
	    /* XXX: um, well we've just wasted some space in pconf ? */
	    return;
	}
    }
}

/* test to ensure that the write_fds are all still writable, otherwise
 * invoke the maintenance functions as appropriate */
static void probe_writable_fds(void)
{
    fd_set writable_fds;
    int fd_max;
    other_child_rec *ocr, *nocr;
    struct timeval tv;
    int rc;

    if (other_children == NULL)
	return;

    fd_max = 0;
    FD_ZERO(&writable_fds);
    for (ocr = other_children; ocr; ocr = ocr->next) {
	if (ocr->write_fd == -1)
	    continue;
	FD_SET(ocr->write_fd, &writable_fds);
	if (ocr->write_fd > fd_max) {
	    fd_max = ocr->write_fd;
	}
    }
    if (fd_max == 0)
	return;

    do {
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	rc = ap_select(fd_max + 1, NULL, &writable_fds, NULL, &tv);
    } while (rc == -1 && errno == EINTR);

    if (rc == -1) {
	/* XXX: uhh this could be really bad, we could have a bad file
	 * descriptor due to a bug in one of the maintenance routines */
	log_unixerr("probe_writable_fds", "select",
		    "could not probe writable fds", server_conf);
	return;
    }
    if (rc == 0)
	return;

    for (ocr = other_children; ocr; ocr = nocr) {
	nocr = ocr->next;
	if (ocr->write_fd == -1)
	    continue;
	if (FD_ISSET(ocr->write_fd, &writable_fds))
	    continue;
	(*ocr->maintenance) (OC_REASON_UNWRITABLE, ocr->data, -1);
    }
}

/* possibly reap an other_child, return 0 if yes, -1 if not */
static int reap_other_child(int pid, int status)
{
    other_child_rec *ocr, *nocr;

    for (ocr = other_children; ocr; ocr = nocr) {
	nocr = ocr->next;
	if (ocr->pid != pid)
	    continue;
	ocr->pid = -1;
	(*ocr->maintenance) (OC_REASON_DEATH, ocr->data, status);
	return 0;
    }
    return -1;
}
#endif

/*****************************************************************
 *
 * Dealing with the scoreboard... a lot of these variables are global
 * only to avoid getting clobbered by the longjmp() that happens when
 * a hard timeout expires...
 *
 * We begin with routines which deal with the file itself... 
 */

#ifdef MULTITHREAD
/*
 * In the multithreaded mode, have multiple threads - not multiple
 * processes that need to talk to each other. Just use a simple
 * malloc. But let the routines that follow, think that you have
 * shared memory (so they use memcpy etc.)
 */
#undef USE_MMAP_SCOREBOARD
#define USE_MMAP_SCOREBOARD 1

void reinit_scoreboard(pool *p)
{
    ap_assert(!scoreboard_image);
    scoreboard_image = (scoreboard *) malloc(SCOREBOARD_SIZE);
    memset(scoreboard_image, 0, SCOREBOARD_SIZE);
}

void cleanup_scoreboard()
{
    ap_assert(scoreboard_image);
    free(scoreboard_image);
    scoreboard_image = NULL;
}

API_EXPORT(void) sync_scoreboard_image()
{
}


#else /* MULTITHREAD */
#if defined(USE_MMAP_SCOREBOARD)

#ifdef QNX
static void cleanup_shared_mem(void *d)
{
    shm_unlink(scoreboard_fname);
}
#endif

static void setup_shared_mem(pool *p)
{
    caddr_t m;

#ifdef __EMX__
    char errstr[MAX_STRING_LEN];
    int rc;

    m = (caddr_t) create_shared_heap("\\SHAREMEM\\SCOREBOARD", SCOREBOARD_SIZE);
    if (m == 0) {
	fprintf(stderr, "httpd: Could not create OS/2 Shared memory pool.\n");
	exit(1);
    }

    rc = _uopen((Heap_t) m);
    if (rc != 0) {
	fprintf(stderr, "httpd: Could not uopen() newly created OS/2 Shared memory pool.\n");
    }

#elif defined(QNX)
/* 
 * POSIX 1003.4 style
 *
 * Note 1: 
 * As of version 4.23A, shared memory in QNX must reside under /dev/shmem,
 * where no subdirectories allowed.
 *
 * POSIX shm_open() and shm_unlink() will take care about this issue,
 * but to avoid confusion, I suggest to redefine scoreboard file name
 * in httpd.conf to cut "logs/" from it. With default setup actual name
 * will be "/dev/shmem/logs.apache_status". 
 * 
 * If something went wrong and Apache did not unlinked this object upon
 * exit, you can remove it manually, using "rm -f" command.
 * 
 * Note 2:
 * <sys/mman.h> in QNX defines MAP_ANON, but current implementation 
 * does NOT support BSD style anonymous mapping. So, the order of 
 * conditional compilation is important: 
 * this #ifdef section must be ABOVE the next one (BSD style).
 *
 * I tested this stuff and it works fine for me, but if it provides 
 * trouble for you, just comment out USE_MMAP_SCOREBOARD in QNX section
 * of conf.h
 *
 * June 5, 1997, 
 * Igor N. Kovalenko -- infoh@mail.wplus.net
 */
    int fd;

    fd = shm_open(scoreboard_fname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
	perror("httpd: could not open(create) scoreboard");
	exit(1);
    }
    if (ltrunc(fd, (off_t) SCOREBOARD_SIZE, SEEK_SET) == -1) {
	perror("httpd: could not ltrunc scoreboard");
	shm_unlink(scoreboard_fname);
	exit(1);
    }
    if ((m = (caddr_t) mmap((caddr_t) 0,
			    (size_t) SCOREBOARD_SIZE, PROT_READ | PROT_WRITE,
			    MAP_SHARED, fd, (off_t) 0)) == (caddr_t) - 1) {
	perror("httpd: cannot mmap scoreboard");
	shm_unlink(scoreboard_fname);
	exit(1);
    }
    close(fd);
    register_cleanup(p, NULL, cleanup_shared_mem, null_cleanup);

#elif defined(MAP_ANON) || defined(MAP_FILE)
/* BSD style */
#ifdef CONVEXOS11
    /*
     * 9-Aug-97 - Jeff Venters (venters@convex.hp.com)
     * ConvexOS maps address space as follows:
     *   0x00000000 - 0x7fffffff : Kernel
     *   0x80000000 - 0xffffffff : User
     * Start mmapped area 1GB above start of text.
     *
     * Also, the length requires a pointer as the actual length is
     * returned (rounded up to a page boundary).
     */
    {
	unsigned len = SCOREBOARD_SIZE;

	m = mmap((caddr_t) 0xC0000000, &len,
		 PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, NOFD, 0);
    }
#else
    m = mmap((caddr_t) 0, SCOREBOARD_SIZE,
	     PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
#endif
    if (m == (caddr_t) - 1) {
	perror("mmap");
	fprintf(stderr, "httpd: Could not mmap memory\n");
	exit(1);
    }
#else
/* Sun style */
    int fd;

    fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
	perror("open");
	fprintf(stderr, "httpd: Could not open /dev/zero\n");
	exit(1);
    }
    m = mmap((caddr_t) 0, SCOREBOARD_SIZE,
	     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == (caddr_t) - 1) {
	perror("mmap");
	fprintf(stderr, "httpd: Could not mmap /dev/zero\n");
	exit(1);
    }
    close(fd);
#endif
    scoreboard_image = (scoreboard *) m;
    scoreboard_image->global.exit_generation = 0;
}

#elif defined(USE_SHMGET_SCOREBOARD)
static key_t shmkey = IPC_PRIVATE;
static int shmid = -1;

static void setup_shared_mem(pool *p)
{
    struct shmid_ds shmbuf;
#ifdef MOVEBREAK
    char *obrk;
#endif

    if ((shmid = shmget(shmkey, SCOREBOARD_SIZE, IPC_CREAT | SHM_R | SHM_W)) == -1) {
#ifdef LINUX
	if (errno == ENOSYS) {
	    aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_EMERG, server_conf,
		    "httpd: Your kernel was built without CONFIG_SYSVIPC\n"
		    "httpd: please consult the Apache FAQ for details\n");
	}
#endif
	aplog_error(APLOG_MARK, APLOG_EMERG, server_conf,
		    "could not call shmget");
	exit(1);
    }

    aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, server_conf,
		"created shared memory segment #%d", shmid);

#ifdef MOVEBREAK
    /*
     * Some SysV systems place the shared segment WAY too close
     * to the dynamic memory break point (sbrk(0)). This severely
     * limits the use of malloc/sbrk in the program since sbrk will
     * refuse to move past that point.
     *
     * To get around this, we move the break point "way up there",
     * attach the segment and then move break back down. Ugly
     */
    if ((obrk = sbrk(MOVEBREAK)) == (char *) -1) {
	aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
	    "sbrk() could not move break");
    }
#endif

#define BADSHMAT	((scoreboard *)(-1))
    if ((scoreboard_image = (scoreboard *) shmat(shmid, 0, 0)) == BADSHMAT) {
	aplog_error(APLOG_MARK, APLOG_EMERG, server_conf, "shmat error");
	/*
	 * We exit below, after we try to remove the segment
	 */
    }
    else {			/* only worry about permissions if we attached the segment */
	if (shmctl(shmid, IPC_STAT, &shmbuf) != 0) {
	    aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
		"shmctl() could not stat segment #%d", shmid);
	}
	else {
	    shmbuf.shm_perm.uid = user_id;
	    shmbuf.shm_perm.gid = group_id;
	    if (shmctl(shmid, IPC_SET, &shmbuf) != 0) {
		aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
		    "shmctl() could not set segment #%d", shmid);
	    }
	}
    }
    /*
     * We must avoid leaving segments in the kernel's
     * (small) tables.
     */
    if (shmctl(shmid, IPC_RMID, NULL) != 0) {
	aplog_error(APLOG_MARK, APLOG_WARNING, server_conf,
		"shmctl: IPC_RMID: could not remove shared memory segment #%d",
		shmid);
    }
    if (scoreboard_image == BADSHMAT)	/* now bailout */
	exit(1);

#ifdef MOVEBREAK
    if (obrk == (char *) -1)
	return;			/* nothing else to do */
    if (sbrk(-(MOVEBREAK)) == (char *) -1) {
	aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
	    "sbrk() could not move break back");
    }
#endif
    scoreboard_image->global.exit_generation = 0;
}

#else
#define SCOREBOARD_FILE
static scoreboard _scoreboard_image;
static int scoreboard_fd = -1;

/* XXX: things are seriously screwed if we ever have to do a partial
 * read or write ... we could get a corrupted scoreboard
 */
static int force_write(int fd, void *buffer, int bufsz)
{
    int rv, orig_sz = bufsz;

    do {
	rv = write(fd, buffer, bufsz);
	if (rv > 0) {
	    buffer = (char *) buffer + rv;
	    bufsz -= rv;
	}
    } while ((rv > 0 && bufsz > 0) || (rv == -1 && errno == EINTR));

    return rv < 0 ? rv : orig_sz - bufsz;
}

static int force_read(int fd, void *buffer, int bufsz)
{
    int rv, orig_sz = bufsz;

    do {
	rv = read(fd, buffer, bufsz);
	if (rv > 0) {
	    buffer = (char *) buffer + rv;
	    bufsz -= rv;
	}
    } while ((rv > 0 && bufsz > 0) || (rv == -1 && errno == EINTR));

    return rv < 0 ? rv : orig_sz - bufsz;
}

static void cleanup_scoreboard_file(void *foo)
{
    unlink(scoreboard_fname);
}
#endif

/* Called by parent process */
void reinit_scoreboard(pool *p)
{
    int exit_gen = 0;
    if (scoreboard_image)
	exit_gen = scoreboard_image->global.exit_generation;

#ifndef SCOREBOARD_FILE
    if (scoreboard_image == NULL) {
	setup_shared_mem(p);
    }
    memset(scoreboard_image, 0, SCOREBOARD_SIZE);
    scoreboard_image->global.exit_generation = exit_gen;
#else
    scoreboard_image = &_scoreboard_image;
    scoreboard_fname = server_root_relative(p, scoreboard_fname);

    scoreboard_fd = popenf(p, scoreboard_fname, O_CREAT | O_BINARY | O_RDWR, 0644);
    if (scoreboard_fd == -1) {
	perror(scoreboard_fname);
	fprintf(stderr, "Cannot open scoreboard file:\n");
	exit(1);
    }
    register_cleanup(p, NULL, cleanup_scoreboard_file, null_cleanup);

    memset((char *) scoreboard_image, 0, sizeof(*scoreboard_image));
    scoreboard_image->global.exit_generation = exit_gen;
    force_write(scoreboard_fd, scoreboard_image, sizeof(*scoreboard_image));
#endif
}

/* called by child */
void reopen_scoreboard(pool *p)
{
#ifdef SCOREBOARD_FILE
    if (scoreboard_fd != -1)
	pclosef(p, scoreboard_fd);

    scoreboard_fd = popenf(p, scoreboard_fname, O_CREAT | O_BINARY | O_RDWR, 0666);
    if (scoreboard_fd == -1) {
	perror(scoreboard_fname);
	fprintf(stderr, "Cannot open scoreboard file:\n");
	clean_child_exit(1);
    }
#else
#ifdef __EMX__
#ifdef USE_MMAP_SCOREBOARD
    caddr_t m;
    int rc;

    m = (caddr_t) get_shared_heap("\\SHAREMEM\\SCOREBOARD");
    if (m == 0) {
	fprintf(stderr, "httpd: Could not find existing OS/2 Shared memory pool.\n");
	exit(1);
    }

    rc = _uopen((Heap_t) m);
    scoreboard_image = (scoreboard *) m;
#endif
#endif
#endif
}

/* Routines called to deal with the scoreboard image
 * --- note that we do *not* need write locks, since update_child_status
 * only updates a *single* record in place, and only one process writes to
 * a given scoreboard slot at a time (either the child process owning that
 * slot, or the parent, noting that the child has died).
 *
 * As a final note --- setting the score entry to getpid() is always safe,
 * since when the parent is writing an entry, it's only noting SERVER_DEAD
 * anyway.
 */

ap_inline void sync_scoreboard_image(void)
{
#ifdef SCOREBOARD_FILE
    lseek(scoreboard_fd, 0L, 0);
    force_read(scoreboard_fd, scoreboard_image, sizeof(*scoreboard_image));
#endif
}

#endif /* MULTITHREAD */

API_EXPORT(int) exists_scoreboard_image(void)
{
    return (scoreboard_image ? 1 : 0);
}

static ap_inline void put_scoreboard_info(int child_num,
				       short_score *new_score_rec)
{
#ifdef SCOREBOARD_FILE
    lseek(scoreboard_fd, (long) child_num * sizeof(short_score), 0);
    force_write(scoreboard_fd, new_score_rec, sizeof(short_score));
#endif
}

/* a clean exit from the parent with proper cleanup */
static void __attribute__((noreturn)) clean_parent_exit(int code)
{
    /* Clear the pool - including any registered cleanups */
    destroy_pool(pconf);
    exit(code);
}

int update_child_status(int child_num, int status, request_rec *r)
{
    int old_status;
    short_score *ss;

    if (child_num < 0)
	return -1;

    sync_scoreboard_image();
    ss = &scoreboard_image->servers[child_num];
    old_status = ss->status;
    ss->status = status;
#ifdef OPTIMIZE_TIMEOUTS
    ++ss->cur_vtime;
#endif

#if defined(STATUS)
#ifndef OPTIMIZE_TIMEOUTS
    ss->last_used = time(NULL);
#endif
    if (status == SERVER_READY || status == SERVER_DEAD) {
	/*
	 * Reset individual counters
	 */
	if (status == SERVER_DEAD) {
	    ss->my_access_count = 0L;
	    ss->my_bytes_served = 0L;
	}
	ss->conn_count = (unsigned short) 0;
	ss->conn_bytes = (unsigned long) 0;
    }
    if (r) {
	conn_rec *c = r->connection;
	ap_cpystrn(ss->client, get_remote_host(c, r->per_dir_config,
			      REMOTE_NOLOOKUP), sizeof(ss->client));
	ap_cpystrn(ss->request, (r->the_request ? r->the_request :
			      "NULL"), sizeof(ss->request));
	ap_cpystrn(ss->vhost, r->server->server_hostname, sizeof(ss->vhost));
    }
#endif

    put_scoreboard_info(child_num, ss);

    return old_status;
}

static void update_scoreboard_global(void)
{
#ifdef SCOREBOARD_FILE
    lseek(scoreboard_fd,
	  (char *) &scoreboard_image->global -(char *) scoreboard_image, 0);
    force_write(scoreboard_fd, &scoreboard_image->global,
		sizeof scoreboard_image->global);
#endif
}

#if defined(STATUS)
void time_process_request(int child_num, int status)
{
    short_score *ss;
#if defined(NO_GETTIMEOFDAY) && !defined(NO_TIMES)
    struct tms tms_blk;
#endif

    if (child_num < 0)
	return;

    sync_scoreboard_image();
    ss = &scoreboard_image->servers[child_num];

    if (status == START_PREQUEST) {
#if defined(NO_GETTIMEOFDAY)
#ifndef NO_TIMES
	if ((ss->start_time = times(&tms_blk)) == -1)
#endif /* NO_TIMES */
	    ss->start_time = (clock_t) 0;
#else
	if (gettimeofday(&ss->start_time, (struct timezone *) 0) < 0)
	    ss->start_time.tv_sec =
		ss->start_time.tv_usec = 0L;
#endif
    }
    else if (status == STOP_PREQUEST) {
#if defined(NO_GETTIMEOFDAY)
#ifndef NO_TIMES
	if ((ss->stop_time = times(&tms_blk)) == -1)
#endif
	    ss->stop_time = ss->start_time = (clock_t) 0;
#else
	if (gettimeofday(&ss->stop_time, (struct timezone *) 0) < 0)
	    ss->stop_time.tv_sec =
		ss->stop_time.tv_usec =
		ss->start_time.tv_sec =
		ss->start_time.tv_usec = 0L;
#endif

    }

    put_scoreboard_info(child_num, ss);
}

static void increment_counts(int child_num, request_rec *r)
{
    long int bs = 0;
    short_score *ss;

    sync_scoreboard_image();
    ss = &scoreboard_image->servers[child_num];

    if (r->sent_bodyct)
	bgetopt(r->connection->client, BO_BYTECT, &bs);

#ifndef NO_TIMES
    times(&ss->times);
#endif
    ss->access_count++;
    ss->my_access_count++;
    ss->conn_count++;
    ss->bytes_served += (unsigned long) bs;
    ss->my_bytes_served += (unsigned long) bs;
    ss->conn_bytes += (unsigned long) bs;

    put_scoreboard_info(child_num, ss);
}

#endif


static int find_child_by_pid(int pid)
{
    int i;

    for (i = 0; i < max_daemons_limit; ++i)
	if (scoreboard_image->parent[i].pid == pid)
	    return i;

    return -1;
}

static void reclaim_child_processes(int terminate)
{
#ifndef MULTITHREAD
    int i, status;
    long int waittime = 1024 * 16;	/* in usecs */
    struct timeval tv;
    int waitret, tries;
    int not_dead_yet;
#ifndef NO_OTHER_CHILD
    other_child_rec *ocr, *nocr;
#endif

    sync_scoreboard_image();

    for (tries = terminate ? 4 : 1; tries <= 9; ++tries) {
	/* don't want to hold up progress any more than 
	 * necessary, but we need to allow children a few moments to exit.
	 * Set delay with an exponential backoff.
	 */
	tv.tv_sec = waittime / 1000000;
	tv.tv_usec = waittime % 1000000;
	waittime = waittime * 4;
	ap_select(0, NULL, NULL, NULL, &tv);

	/* now see who is done */
	not_dead_yet = 0;
	for (i = 0; i < max_daemons_limit; ++i) {
	    int pid = scoreboard_image->parent[i].pid;

	    if (pid == my_pid || pid == 0)
		continue;

	    waitret = waitpid(pid, &status, WNOHANG);
	    if (waitret == pid || waitret == -1) {
		scoreboard_image->parent[i].pid = 0;
		continue;
	    }
	    ++not_dead_yet;
	    switch (tries) {
	    case 1:     /*  16ms */
	    case 2:     /*  82ms */
		break;
	    case 3:     /* 344ms */
		/* perhaps it missed the SIGHUP, lets try again */
		aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_WARNING,
			    server_conf,
		    "child process %d did not exit, sending another SIGHUP",
			    pid);
		kill(pid, SIGHUP);
		waittime = 1024 * 16;
		break;
	    case 4:     /*  16ms */
	    case 5:     /*  82ms */
	    case 6:     /* 344ms */
		break;
	    case 7:     /* 1.4sec */
		/* ok, now it's being annoying */
		aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_WARNING,
			    server_conf,
		   "child process %d still did not exit, sending a SIGTERM",
			    pid);
		kill(pid, SIGTERM);
		break;
	    case 8:     /*  6 sec */
		/* die child scum */
		aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, server_conf,
		   "child process %d still did not exit, sending a SIGKILL",
			    pid);
		kill(pid, SIGKILL);
		break;
	    case 9:     /* 14 sec */
		/* gave it our best shot, but alas...  If this really 
		 * is a child we are trying to kill and it really hasn't
		 * exited, we will likely fail to bind to the port
		 * after the restart.
		 */
		aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, server_conf,
			    "could not make child process %d exit, "
			    "attempting to continue anyway", pid);
		break;
	    }
	}
#ifndef NO_OTHER_CHILD
	for (ocr = other_children; ocr; ocr = nocr) {
	    nocr = ocr->next;
	    if (ocr->pid == -1)
		continue;

	    waitret = waitpid(ocr->pid, &status, WNOHANG);
	    if (waitret == ocr->pid) {
		ocr->pid = -1;
		(*ocr->maintenance) (OC_REASON_DEATH, ocr->data, status);
	    }
	    else if (waitret == 0) {
		(*ocr->maintenance) (OC_REASON_RESTART, ocr->data, -1);
		++not_dead_yet;
	    }
	    else if (waitret == -1) {
		/* uh what the heck? they didn't call unregister? */
		ocr->pid = -1;
		(*ocr->maintenance) (OC_REASON_LOST, ocr->data, -1);
	    }
	}
#endif
	if (!not_dead_yet) {
	    /* nothing left to wait for */
	    break;
	}
    }
#endif /* ndef MULTITHREAD */
}


#if defined(BROKEN_WAIT) || defined(NEED_WAITPID)
/*
   Some systems appear to fail to deliver dead children to wait() at times.
   This sorts them out. In fact, this may have been caused by a race condition
   in wait_or_timeout(). But this routine is still useful for systems with no
   waitpid().
 */
int reap_children(void)
{
    int status, n;
    int ret = 0;

    for (n = 0; n < max_daemons_limit; ++n) {
	if (scoreboard_image->servers[n].status != SERVER_DEAD
	    && waitpid(scoreboard_image->parent[n].pid, &status, WNOHANG)
	    == -1
	    && errno == ECHILD) {
	    sync_scoreboard_image();
	    update_child_status(n, SERVER_DEAD, NULL);
	    ret = 1;
	}
    }
    return ret;
}
#endif

/* Finally, this routine is used by the caretaker process to wait for
 * a while...
 */

/* number of calls to wait_or_timeout between writable probes */
#ifndef INTERVAL_OF_WRITABLE_PROBES
#define INTERVAL_OF_WRITABLE_PROBES 10
#endif
static int wait_or_timeout_counter;

static int wait_or_timeout(int *status)
{
#ifdef WIN32
#define MAXWAITOBJ MAXIMUM_WAIT_OBJECTS
    HANDLE h[MAXWAITOBJ];
    int e[MAXWAITOBJ];
    int round, pi, hi, rv, err;
    for (round = 0; round <= (HARD_SERVER_LIMIT - 1) / MAXWAITOBJ + 1; round++) {
	hi = 0;
	for (pi = round * MAXWAITOBJ;
	     (pi < (round + 1) * MAXWAITOBJ) && (pi < HARD_SERVER_LIMIT);
	     pi++) {
	    if (scoreboard_image->servers[pi].status != SERVER_DEAD) {
		e[hi] = pi;
		h[hi++] = (HANDLE) scoreboard_image->parent[pi].pid;
	    }

	}
	if (hi > 0) {
	    rv = WaitForMultipleObjects(hi, h, FALSE, 10000);
	    if (rv == -1)
		err = GetLastError();
	    if ((WAIT_OBJECT_0 <= (unsigned int) rv) && ((unsigned int) rv < (WAIT_OBJECT_0 + hi)))
		return (scoreboard_image->parent[e[rv - WAIT_OBJECT_0]].pid);
	    else if ((WAIT_ABANDONED_0 <= (unsigned int) rv) && ((unsigned int) rv < (WAIT_ABANDONED_0 + hi)))
		return (scoreboard_image->parent[e[rv - WAIT_ABANDONED_0]].pid);

	}
    }
    return (-1);

#else /* WIN32 */
    struct timeval tv;
    int ret;

    ++wait_or_timeout_counter;
    if (wait_or_timeout_counter == INTERVAL_OF_WRITABLE_PROBES) {
	wait_or_timeout_counter = 0;
#ifndef NO_OTHER_CHILD
	probe_writable_fds();
#endif
    }
    ret = waitpid(-1, status, WNOHANG);
    if (ret == -1 && errno == EINTR) {
	return -1;
    }
    if (ret > 0) {
	return ret;
    }
    tv.tv_sec = SCOREBOARD_MAINTENANCE_INTERVAL / 1000000;
    tv.tv_usec = SCOREBOARD_MAINTENANCE_INTERVAL % 1000000;
    ap_select(0, NULL, NULL, NULL, &tv);
    return -1;
#endif /* WIN32 */
}


/* handle all varieties of core dumping signals */
void sig_coredump(int sig)
{
    char emsg[256];
    const char *s;

    /* Must protect against a nested signal, otherwise we could end up in
     * an infinite loop.
     */
    signal(SIGSEGV, SIG_DFL);
#ifdef SIGBUS
    signal(SIGBUS, SIG_DFL);
#endif
#ifdef SIGABORT
    signal(SIGABORT, SIG_DFL);
#endif
#ifdef SIGABRT
    signal(SIGABRT, SIG_DFL);
#endif

    s = "SIGSEGV";
#ifdef SIGBUS
    if (sig == SIGBUS) {
	s = "SIGBUS";
    }
#endif
#ifdef SIGABORT
    if (sig == SIGABORT) {
	s = "SIGABORT";
    }
#endif
#ifdef SIGABRT
    if (sig == SIGABRT) {
	s = "SIGABRT";
    }
#endif

    ap_snprintf(emsg, sizeof(emsg),
		"httpd: caught %s, attempting to dump core in %s",
		s, coredump_dir);
    aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_NOTICE, server_conf, emsg);
    chdir(coredump_dir);
    abort();
    exit(1);
}

/*****************************************************************
 * Connection structures and accounting...
 */

void just_die(int sig)
{				/* SIGHUP to child process??? */
    /* if alarms are blocked we have to wait to die otherwise we might
     * end up with corruption in alloc.c's internal structures */
    if (alarms_blocked) {
	exit_after_unblock = 1;
    }
    else {
	clean_child_exit(0);
    }
}

static int volatile usr1_just_die = 1;
static int volatile deferred_die;

static void usr1_handler(int sig)
{
    if (usr1_just_die) {
	just_die(sig);
    }
    deferred_die = 1;
}

/* volatile just in case */
static int volatile shutdown_pending;
static int volatile restart_pending;
static int volatile is_graceful;
static int volatile generation;

#ifdef WIN32
static void signal_parent(void)
{
    HANDLE e;
    /* after updating the shutdown_pending or restart flags, we need
     * to wake up the parent process so it can see the changes. The
     * parent will normally be waiting for either a child process
     * to die, or for a signal on the "spache-signal" event. So set the
     * "apache-signal" event here.
     */

    APD1("*** SIGNAL_PARENT SET ***");

    e = OpenEvent(EVENT_ALL_ACCESS, FALSE, "apache-signal");
    if (!e) {
	/* Um, problem, can't signal the main loop, which means we can't
	 * signal ourselves to die. Ignore for now...
	 */
	aplog_error(APLOG_MARK, APLOG_EMERG|APLOG_WIN32ERROR, server_conf,
	    "OpenEvent on apache-signal event");
	return;
    }
    if (SetEvent(e) == 0) {
	/* Same problem as above */
	aplog_error(APLOG_MARK, APLOG_EMERG|APLOG_WIN32ERROR, server_conf,
	    "SetEvent on apache-signal event");
	return;
    }
    CloseHandle(e);
}
#endif

/*
 * start_shutdown() and start_restart(), below, are a first stab at
 * functions to initiate shutdown or restart without relying on signals. 
 * Previously this was initiated in sig_term() and restart() signal handlers, 
 * but we want to be able to start a shutdown/restart from other sources --
 * e.g. on Win32, from the service manager. Now the service manager can
 * call start_shutdown() or start_restart() as appropiate. 
 */

void start_shutdown(void)
{
    if (shutdown_pending == 1) {
	/* Um, is this _probably_ not an error, if the user has
	 * tried to do a shutdown twice quickly, so we won't
	 * worry about reporting it.
	 */
	return;
    }
    shutdown_pending = 1;

#ifdef WIN32
    signal_parent();	    /* get the parent process to wake up */
#endif
}

/* do a graceful restart if graceful == 1 */
void start_restart(int graceful)
{
    if (restart_pending == 1) {
	/* Probably not an error - don't bother reporting it */
	return;
    }
    restart_pending = 1;
    is_graceful = graceful;

#ifdef WIN32
    signal_parent();	    /* get the parent process to wake up */
#endif /* WIN32 */
}

static void sig_term(int sig)
{
    start_shutdown();
}

static void restart(int sig)
{
#ifndef WIN32
    start_restart(sig == SIGUSR1);
#else
    start_restart(1);
#endif
}

void set_signals(void)
{
#ifndef NO_USE_SIGACTION
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (!one_process) {
	sa.sa_handler = sig_coredump;
	if (sigaction(SIGSEGV, &sa, NULL) < 0)
	    aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "sigaction(SIGSEGV)");
#ifdef SIGBUS
	if (sigaction(SIGBUS, &sa, NULL) < 0)
	    aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "sigaction(SIGBUS)");
#endif
#ifdef SIGABORT
	if (sigaction(SIGABORT, &sa, NULL) < 0)
	    aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "sigaction(SIGABORT)");
#endif
#ifdef SIGABRT
	if (sigaction(SIGABRT, &sa, NULL) < 0)
	    aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "sigaction(SIGABRT)");
#endif
    }
    sa.sa_handler = sig_term;
    if (sigaction(SIGTERM, &sa, NULL) < 0)
	aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "sigaction(SIGTERM)");
#ifdef SIGINT
    if (sigaction(SIGINT, &sa, NULL) < 0)
        aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "sigaction(SIGINT)");
#endif

    /* we want to ignore HUPs and USR1 while we're busy processing one */
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sa.sa_handler = restart;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
	aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "sigaction(SIGHUP)");
    if (sigaction(SIGUSR1, &sa, NULL) < 0)
	aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "sigaction(SIGUSR1)");
#else
    if (!one_process) {
	signal(SIGSEGV, sig_coredump);
#ifdef SIGBUS
	signal(SIGBUS, sig_coredump);
#endif /* SIGBUS */
#ifdef SIGABORT
	signal(SIGABORT, sig_coredump);
#endif /* SIGABORT */
#ifdef SIGABRT
	signal(SIGABRT, sig_coredump);
#endif /* SIGABRT */
    }

    signal(SIGTERM, sig_term);
#ifdef SIGHUP
    signal(SIGHUP, restart);
#endif /* SIGHUP */
#ifdef SIGUSR1
    signal(SIGUSR1, restart);
#endif /* SIGUSR1 */
#endif
}


/*****************************************************************
 * Here follows a long bunch of generic server bookkeeping stuff...
 */

void detach(void)
{
#if !defined(WIN32)
    int x;

    chdir("/");
#if !defined(MPE) && !defined(__EMX__)
/* Don't detach for MPE because child processes can't survive the death of
   the parent. */
    if ((x = fork()) > 0)
	exit(0);
    else if (x == -1) {
	perror("fork");
	fprintf(stderr, "httpd: unable to fork new process\n");
	exit(1);
    }
    RAISE_SIGSTOP(DETACH);
#endif
#ifndef NO_SETSID
    if ((pgrp = setsid()) == -1) {
	perror("setsid");
	fprintf(stderr, "httpd: setsid failed\n");
	exit(1);
    }
#elif defined(NEXT) || defined(NEWSOS)
    if (setpgrp(0, getpid()) == -1 || (pgrp = getpgrp(0)) == -1) {
	perror("setpgrp");
	fprintf(stderr, "httpd: setpgrp or getpgrp failed\n");
	exit(1);
    }
#elif defined(__EMX__)
    /* OS/2 don't support process group IDs */
    pgrp = getpid();
#elif defined(MPE)
    /* MPE uses negative pid for process group */
    pgrp = -getpid();
#else
    if ((pgrp = setpgrp(getpid(), 0)) == -1) {
	perror("setpgrp");
	fprintf(stderr, "httpd: setpgrp failed\n");
	exit(1);
    }
#endif

    /* close out the standard file descriptors */
    if (freopen("/dev/null", "r", stdin) == NULL) {
	fprintf(stderr, "httpd: unable to replace stdin with /dev/null: %s\n",
		strerror(errno));
	/* continue anyhow -- note we can't close out descriptor 0 because we
	 * have nothing to replace it with, and if we didn't have a descriptor
	 * 0 the next file would be created with that value ... leading to
	 * havoc.
	 */
    }
    if (freopen("/dev/null", "w", stdout) == NULL) {
	fprintf(stderr, "httpd: unable to replace stdout with /dev/null: %s\n",
		strerror(errno));
    }
    /* stderr is a tricky one, we really want it to be the error_log,
     * but we haven't opened that yet.  So leave it alone for now and it'll
     * be reopened moments later.
     */
#endif /* ndef WIN32 */
}

/* Set group privileges.
 *
 * Note that we use the username as set in the config files, rather than
 * the lookup of to uid --- the same uid may have multiple passwd entries,
 * with different sets of groups for each.
 */

static void set_group_privs(void)
{
#ifndef WIN32
    if (!geteuid()) {
	char *name;

	/* Get username if passed as a uid */

	if (user_name[0] == '#') {
	    struct passwd *ent;
	    uid_t uid = atoi(&user_name[1]);

	    if ((ent = getpwuid(uid)) == NULL) {
		aplog_error(APLOG_MARK, APLOG_ALERT, server_conf,
			 "getpwuid: couldn't determine user name from uid");
		exit(1);
	    }

	    name = ent->pw_name;
	}
	else
	    name = user_name;

#ifndef __EMX__
	/* OS/2 dosen't support groups. */

	/* Reset `groups' attributes. */

	if (initgroups(name, group_id) == -1) {
	    aplog_error(APLOG_MARK, APLOG_ALERT, server_conf,
			"initgroups: unable to set groups");
	    exit(1);
	}
#ifdef MULTIPLE_GROUPS
	if (getgroups(NGROUPS_MAX, group_id_list) == -1) {
	    aplog_error(APLOG_MARK, APLOG_ALERT, server_conf,
			"getgroups: unable to get group list");
	    exit(1);
	}
#endif
	if (setgid(group_id) == -1) {
	    aplog_error(APLOG_MARK, APLOG_ALERT, server_conf,
			"setgid: unable to set group id");
	    exit(1);
	}
#endif
    }
#endif /* ndef WIN32 */
}

/* check to see if we have the 'suexec' setuid wrapper installed */
int init_suexec(void)
{
#ifndef WIN32
    struct stat wrapper;

    if ((stat(SUEXEC_BIN, &wrapper)) != 0)
	return (suexec_enabled);

    if ((wrapper.st_mode & S_ISUID) && wrapper.st_uid == 0) {
	suexec_enabled = 1;
	fprintf(stderr, "Configuring Apache for use with suexec wrapper.\n");
    }
#endif /* ndef WIN32 */
    return (suexec_enabled);
}

/*****************************************************************
 * Connection structures and accounting...
 */


conn_rec *ap__new_connection(pool *p, server_rec *server, BUFF *inout,
			     const struct sockaddr_in *remaddr,
			     const struct sockaddr_in *saddr,
			     int child_num)
{
    conn_rec *conn = (conn_rec *) pcalloc(p, sizeof(conn_rec));

    /* Got a connection structure, so initialize what fields we can
     * (the rest are zeroed out by pcalloc).
     */

    conn->child_num = child_num;

    conn->pool = p;
    conn->local_addr = *saddr;
    conn->server = server; /* just a guess for now */
    update_vhost_given_ip(conn);
    conn->base_server = conn->server;
    conn->client = inout;

    conn->remote_addr = *remaddr;
    conn->remote_ip = pstrdup(conn->pool,
			      inet_ntoa(conn->remote_addr.sin_addr));

    return conn;
}

#if defined(TCP_NODELAY) && !defined(MPE)
static void sock_disable_nagle(int s)
{
    /* The Nagle algorithm says that we should delay sending partial
     * packets in hopes of getting more data.  We don't want to do
     * this; we are not telnet.  There are bad interactions between
     * persistent connections and Nagle's algorithm that have very severe
     * performance penalties.  (Failing to disable Nagle is not much of a
     * problem with simple HTTP.)
     *
     * In spite of these problems, failure here is not a shooting offense.
     */
    int just_say_no = 1;

    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *) &just_say_no,
		   sizeof(int)) < 0) {
	aplog_error(APLOG_MARK, APLOG_WARNING, server_conf,
		    "setsockopt: (TCP_NODELAY)");
    }
}

#else
#define sock_disable_nagle(s)	/* NOOP */
#endif


static int make_sock(pool *p, const struct sockaddr_in *server)
{
    int s;
    int one = 1;

    /* note that because we're about to slack we don't use psocket */
    block_alarms();
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
	aplog_error(APLOG_MARK, APLOG_CRIT, server_conf,
		    "socket: Failed to get a socket, exiting child");
	unblock_alarms();
	exit(1);
    }

    /* Solaris (probably versions 2.4, 2.5, and 2.5.1 with various levels
     * of tcp patches) has some really weird bugs where if you dup the
     * socket now it breaks things across SIGHUP restarts.  It'll either
     * be unable to bind, or it won't respond.
     */
#if defined (SOLARIS2) && SOLARIS2 < 260
#define WORKAROUND_SOLARIS_BUG
#endif

    /* PR#1282 Unixware 1.x appears to have the same problem as solaris */
#if defined (UW) && UW < 200
#define WORKAROUND_SOLARIS_BUG
#endif

#ifndef WORKAROUND_SOLARIS_BUG
    s = ap_slack(s, AP_SLACK_HIGH);

    note_cleanups_for_socket(p, s);	/* arrange to close on exec or restart */
    unblock_alarms();
#endif

#ifndef MPE
/* MPE does not support SO_REUSEADDR and SO_KEEPALIVE */
#ifndef _OSD_POSIX
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(int)) < 0) {
	aplog_error(APLOG_MARK, APLOG_CRIT, server_conf,
		    "setsockopt: (SO_REUSEADDR)");
	exit(1);
    }
#endif /*_OSD_POSIX*/
    one = 1;
#ifndef BEOS
/* BeOS does not support SO_KEEPALIVE */
    if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *) &one, sizeof(int)) < 0) {
	aplog_error(APLOG_MARK, APLOG_CRIT, server_conf,
		    "setsockopt: (SO_KEEPALIVE)");
	exit(1);
    }
#endif
#endif

    sock_disable_nagle(s);
    sock_enable_linger(s);

    /*
     * To send data over high bandwidth-delay connections at full
     * speed we must force the TCP window to open wide enough to keep the
     * pipe full.  The default window size on many systems
     * is only 4kB.  Cross-country WAN connections of 100ms
     * at 1Mb/s are not impossible for well connected sites.
     * If we assume 100ms cross-country latency,
     * a 4kB buffer limits throughput to 40kB/s.
     *
     * To avoid this problem I've added the SendBufferSize directive
     * to allow the web master to configure send buffer size.
     *
     * The trade-off of larger buffers is that more kernel memory
     * is consumed.  YMMV, know your customers and your network!
     *
     * -John Heidemann <johnh@isi.edu> 25-Oct-96
     *
     * If no size is specified, use the kernel default.
     */
#ifndef BEOS			/* BeOS does not support SO_SNDBUF */
    if (server_conf->send_buffer_size) {
	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,
		(char *) &server_conf->send_buffer_size, sizeof(int)) < 0) {
	    aplog_error(APLOG_MARK, APLOG_WARNING, server_conf,
			"setsockopt: (SO_SNDBUF): Failed to set SendBufferSize, using default");
	    /* not a fatal error */
	}
    }
#endif

#ifdef MPE
/* MPE requires CAP=PM and GETPRIVMODE to bind to ports less than 1024 */
    if (ntohs(server->sin_port) < 1024)
	GETPRIVMODE();
#endif
    if (bind(s, (struct sockaddr *) server, sizeof(struct sockaddr_in)) == -1) {
	perror("bind");
#ifdef MPE
	if (ntohs(server->sin_port) < 1024)
	    GETUSERMODE();
#endif
	if (server->sin_addr.s_addr != htonl(INADDR_ANY))
	    fprintf(stderr, "httpd: could not bind to address %s port %d\n",
		    inet_ntoa(server->sin_addr), ntohs(server->sin_port));
	else
	    fprintf(stderr, "httpd: could not bind to port %d\n",
		    ntohs(server->sin_port));
	exit(1);
    }
#ifdef MPE
    if (ntohs(server->sin_port) < 1024)
	GETUSERMODE();
#endif

    if (listen(s, listenbacklog) == -1) {
	aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
		    "listen: unable to listen for connections");
	close(s);
#ifdef WORKAROUND_SOLARIS_BUG
	unblock_alarms();
#endif
	return -1;
    }

#ifdef WORKAROUND_SOLARIS_BUG
    s = ap_slack(s, AP_SLACK_HIGH);

    note_cleanups_for_socket(p, s);	/* arrange to close on exec or restart */
    unblock_alarms();
#endif
    return s;
}


/*
 * During a restart we keep track of the old listeners here, so that we
 * can re-use the sockets.  We have to do this because we won't be able
 * to re-open the sockets ("Address already in use").
 *
 * Unlike the listeners ring, old_listeners is a NULL terminated list.
 *
 * copy_listeners() makes the copy, find_listener() finds an old listener
 * and close_unused_listener() cleans up whatever wasn't used.
 */
static listen_rec *old_listeners;

/* unfortunately copy_listeners may be called before listeners is a ring */
static void copy_listeners(pool *p)
{
    listen_rec *lr;

    ap_assert(old_listeners == NULL);
    if (listeners == NULL) {
	return;
    }
    lr = listeners;
    do {
	listen_rec *nr = malloc(sizeof *nr);
	if (nr == NULL) {
	    fprintf(stderr, "Ouch!  malloc failed in copy_listeners()\n");
	    exit(1);
	}
	*nr = *lr;
	kill_cleanups_for_socket(p, nr->fd);
	nr->next = old_listeners;
	ap_assert(!nr->used);
	old_listeners = nr;
	lr = lr->next;
    } while (lr && lr != listeners);
}


static int find_listener(listen_rec *lr)
{
    listen_rec *or;

    for (or = old_listeners; or; or = or->next) {
	if (!memcmp(&or->local_addr, &lr->local_addr, sizeof(or->local_addr))) {
	    or->used = 1;
	    return or->fd;
	}
    }
    return -1;
}


static void close_unused_listeners(void)
{
    listen_rec *or, *next;

    for (or = old_listeners; or; or = next) {
	next = or->next;
	if (!or->used)
	    closesocket(or->fd);
	free(or);
    }
    old_listeners = NULL;
}


/* open sockets, and turn the listeners list into a singly linked ring */
static void setup_listeners(pool *p)
{
    listen_rec *lr;
    int fd;

    listenmaxfd = -1;
    FD_ZERO(&listenfds);
    lr = listeners;
    for (;;) {
	fd = find_listener(lr);
	if (fd < 0) {
	    fd = make_sock(p, &lr->local_addr);
	}
	if (fd >= 0) {
	    FD_SET(fd, &listenfds);
	    if (fd > listenmaxfd)
		listenmaxfd = fd;
	}
	lr->fd = fd;
	if (lr->next == NULL)
	    break;
	lr = lr->next;
    }
    /* turn the list into a ring */
    lr->next = listeners;
    head_listener = listeners;
    close_unused_listeners();

#ifdef NO_SERIALIZED_ACCEPT
    /* warn them about the starvation problem if they're using multiple
     * sockets
     */
    if (listeners->next != listeners) {
	aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_CRIT, NULL,
		    "You cannot use multiple Listens safely on your system, "
		    "proceeding anyway.  See src/PORTING, search for "
		    "SERIALIZED_ACCEPT.");
    }
#endif
}


/*
 * Find a listener which is ready for accept().  This advances the
 * head_listener global.
 */
static ap_inline listen_rec *find_ready_listener(fd_set * main_fds)
{
    listen_rec *lr;

    lr = head_listener;
    do {
	if (FD_ISSET(lr->fd, main_fds)) {
	    head_listener = lr->next;
	    return (lr);
	}
	lr = lr->next;
    } while (lr != head_listener);
    return NULL;
}


static int s_iInitCount = 0;

int AMCSocketInitialize(void)
{
#ifdef WIN32
    int iVersionRequested;
    WSADATA wsaData;
    int err;

    if (s_iInitCount > 0) {
	s_iInitCount++;
	return (0);
    }
    else if (s_iInitCount < 0)
	return (s_iInitCount);

    /* s_iInitCount == 0. Do the initailization */
    iVersionRequested = MAKEWORD(1, 1);
    err = WSAStartup((WORD) iVersionRequested, &wsaData);
    if (err) {
	s_iInitCount = -1;
	return (s_iInitCount);
    }
    if (LOBYTE(wsaData.wVersion) != 1 ||
	HIBYTE(wsaData.wVersion) != 1) {
	s_iInitCount = -2;
	WSACleanup();
	return (s_iInitCount);
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif /* WIN32 */

    s_iInitCount++;
    return (s_iInitCount);

}


void AMCSocketCleanup(void)
{
#ifdef WIN32
    if (--s_iInitCount == 0)
	WSACleanup();
#else /* not WIN32 */
    s_iInitCount--;
#endif /* WIN32 */
    return;
}

static void show_compile_settings(void)
{
#ifdef SERVER_SUBVERSION
    printf("Server base version: \"%s\"\n", SERVER_BASEVERSION);
    printf("Server sub-version:  \"%s\"\n", SERVER_SUBVERSION);
#else
    printf("Server version \"%s\"\n", SERVER_VERSION);
#endif
    printf("Server built:  %s\n", SERVER_BUILT);
    printf("Server's Module Magic Number: %u\n", MODULE_MAGIC_NUMBER);
    printf("Server compiled with....\n");
#ifdef BIG_SECURITY_HOLE
    printf(" -D BIG_SECURITY_HOLE\n");
#endif
#ifdef SECURITY_HOLE_PASS_AUTHORIZATION
    printf(" -D SECURITY_HOLE_PASS_AUTHORIZATION\n");
#endif
#ifdef HTTPD_ROOT
    printf(" -D HTTPD_ROOT=\"" HTTPD_ROOT "\"\n");
#endif
#ifdef HAVE_MMAP
    printf(" -D HAVE_MMAP\n");
#endif
#ifdef HAVE_SHMGET
    printf(" -D HAVE_SHMGET\n");
#endif
#ifdef USE_MMAP_SCOREBOARD
    printf(" -D USE_MMAP_SCOREBOARD\n");
#endif
#ifdef USE_SHMGET_SCOREBOARD
    printf(" -D USE_SHMGET_SCOREBOARD\n");
#endif
#ifdef USE_MMAP_FILES
    printf(" -D USE_MMAP_FILES\n");
#ifdef MMAP_SEGMENT_SIZE
	printf(" -D MMAP_SEGMENT_SIZE=%ld\n",(long)MMAP_SEGMENT_SIZE);
#endif
#endif /*USE_MMAP_FILES*/
#ifdef NO_WRITEV
    printf(" -D NO_WRITEV\n");
#endif
#ifdef NO_LINGCLOSE
    printf(" -D NO_LINGCLOSE\n");
#endif
#ifdef USE_FCNTL_SERIALIZED_ACCEPT
    printf(" -D USE_FCNTL_SERIALIZED_ACCEPT\n");
#endif
#ifdef USE_FLOCK_SERIALIZED_ACCEPT
    printf(" -D USE_FLOCK_SERIALIZED_ACCEPT\n");
#endif
#ifdef USE_USLOCK_SERIALIZED_ACCEPT
    printf(" -D USE_USLOCK_SERIALIZED_ACCEPT\n");
#endif
#ifdef USE_SYSVSEM_SERIALIZED_ACCEPT
    printf(" -D USE_SYSVSEM_SERIALIZED_ACCEPT\n");
#endif
#ifdef USE_PTHREAD_SERIALIZED_ACCEPT
    printf(" -D USE_PTHREAD_SERIALIZED_ACCEPT\n");
#endif
#ifdef SINGLE_LISTEN_UNSERIALIZED_ACCEPT
    printf(" -D SINGLE_LISTEN_UNSERIALIZED_ACCEPT\n");
#endif
#ifdef NO_OTHER_CHILD
    printf(" -D NO_OTHER_CHILD\n");
#endif
#ifdef NO_RELIABLE_PIPED_LOGS
    printf(" -D NO_RELIABLE_PIPED_LOGS\n");
#endif
#ifdef BUFFERED_LOGS
    printf(" -D BUFFERED_LOGS\n");
#ifdef PIPE_BUF
	printf(" -D PIPE_BUF=%ld\n",(long)PIPE_BUF);
#endif
#endif
#ifdef MULTITHREAD
    printf(" -D MULTITHREAD\n");
#endif
#ifdef CHARSET_EBCDIC
    printf(" -D CHARSET_EBCDIC\n");
#endif
#ifdef NEED_HASHBANG_EMUL
    printf(" -D NEED_HASHBANG_EMUL\n");
#endif
    printf("\n");
}

#ifndef MULTITHREAD
/*****************************************************************
 * Child process main loop.
 * The following vars are static to avoid getting clobbered by longjmp();
 * they are really private to child_main.
 */

static int srv;
static int csd;
static int dupped_csd;
static int requests_this_child;
static fd_set main_fds;

API_EXPORT(void) child_terminate(request_rec *r)
{
    r->connection->keepalive = 0;
    requests_this_child = max_requests_per_child = 1;
}

void child_main(int child_num_arg)
{
    NET_SIZE_T clen;
    struct sockaddr sa_server;
    struct sockaddr sa_client;
    listen_rec *lr;

    /* All of initialization is a critical section, we don't care if we're
     * told to HUP or USR1 before we're done initializing.  For example,
     * we could be half way through child_init_modules() when a restart
     * signal arrives, and we'd have no real way to recover gracefully
     * and exit properly.
     *
     * I suppose a module could take forever to initialize, but that would
     * be either a broken module, or a broken configuration (i.e. network
     * problems, file locking problems, whatever). -djg
     */
    block_alarms();

    my_pid = getpid();
    csd = -1;
    dupped_csd = -1;
    my_child_num = child_num_arg;
    requests_this_child = 0;

    /* Get a sub pool for global allocations in this child, so that
     * we can have cleanups occur when the child exits.
     */
    pchild = make_sub_pool(pconf);

    /* needs to be done before we switch UIDs so we have permissions */
    reopen_scoreboard(pchild);
    SAFE_ACCEPT(accept_mutex_child_init(pchild));

    set_group_privs();
#ifdef MPE
    /* Only try to switch if we're running as MANAGER.SYS */
    if (geteuid() == 1 && user_id > 1) {
	GETPRIVMODE();
	if (setuid(user_id) == -1) {
	    GETUSERMODE();
	    aplog_error(APLOG_MARK, APLOG_ALERT, server_conf,
			"setuid: unable to change uid");
	    exit(1);
	}
	GETUSERMODE();
    }
#else
    /* Only try to switch if we're running as root */
    if (!geteuid() && setuid(user_id) == -1) {
	aplog_error(APLOG_MARK, APLOG_ALERT, server_conf,
		    "setuid: unable to change uid");
	clean_child_exit(1);
    }
#endif

    child_init_modules(pchild, server_conf);

    (void) update_child_status(my_child_num, SERVER_READY, (request_rec *) NULL);

    /*
     * Setup the jump buffers so that we can return here after
     * a signal or a timeout (yeah, I know, same thing).
     */
    ap_setjmp(jmpbuffer);
#ifndef __EMX__
#ifdef SIGURG
    signal(SIGURG, timeout);
#endif
#endif
    signal(SIGPIPE, timeout);
    signal(SIGALRM, alrm_handler);

#ifdef __EMX__
/* Stop Ctrl-C/Ctrl-Break signals going to child processes */
    {
        unsigned long ulTimes;
        DosSetSignalExceptionFocus(0, &ulTimes);
    }
#endif

    /* done with the initialization critical section */
    unblock_alarms();

    while (1) {
	BUFF *conn_io;
	request_rec *r;

	/* Prepare to receive a SIGUSR1 due to graceful restart so that
	 * we can exit cleanly.  Since we're between connections right
	 * now it's the right time to exit, but we might be blocked in a
	 * system call when the graceful restart request is made. */
	usr1_just_die = 1;
	signal(SIGUSR1, usr1_handler);

	/*
	 * (Re)initialize this child to a pre-connection state.
	 */

	kill_timeout(0);	/* Cancel any outstanding alarms. */
	timeout_req = NULL;	/* No request in progress */
	current_conn = NULL;

	clear_pool(ptrans);

	sync_scoreboard_image();
	if (scoreboard_image->global.exit_generation >= generation) {
	    clean_child_exit(0);
	}

	if ((max_requests_per_child > 0
	     && ++requests_this_child >= max_requests_per_child)) {
	    clean_child_exit(0);
	}

	(void) update_child_status(my_child_num, SERVER_READY, (request_rec *) NULL);

	/*
	 * Wait for an acceptable connection to arrive.
	 */

	/* Lock around "accept", if necessary */
	SAFE_ACCEPT(accept_mutex_on());

	for (;;) {
	    if (listeners->next != listeners) {
		/* more than one socket */
		memcpy(&main_fds, &listenfds, sizeof(fd_set));
		srv = ap_select(listenmaxfd + 1, &main_fds, NULL, NULL, NULL);

		if (srv < 0 && errno != EINTR) {
		    /* Single Unix documents select as returning errnos
		     * EBADF, EINTR, and EINVAL... and in none of those
		     * cases does it make sense to continue.  In fact
		     * on Linux 2.0.x we seem to end up with EFAULT
		     * occasionally, and we'd loop forever due to it.
		     */
		    aplog_error(APLOG_MARK, APLOG_ERR, server_conf, "select: (listen)");
		    clean_child_exit(1);
		}

		if (srv <= 0)
		    continue;

		lr = find_ready_listener(&main_fds);
		if (lr == NULL)
		    continue;
		sd = lr->fd;
	    }
	    else {
		/* only one socket, just pretend we did the other stuff */
		sd = listeners->fd;
	    }

	    /* if we accept() something we don't want to die, so we have to
	     * defer the exit
	     */
	    deferred_die = 0;
	    usr1_just_die = 0;
	    for (;;) {
		clen = sizeof(sa_client);
		csd = accept(sd, &sa_client, &clen);
		if (csd >= 0 || errno != EINTR)
		    break;
		if (deferred_die) {
		    /* we didn't get a socket, and we were told to die */
		    clean_child_exit(0);
		}
	    }

	    if (csd >= 0)
		break;		/* We have a socket ready for reading */
	    else {

		/* Our old behaviour here was to continue after accept()
		 * errors.  But this leads us into lots of troubles
		 * because most of the errors are quite fatal.  For
		 * example, EMFILE can be caused by slow descriptor
		 * leaks (say in a 3rd party module, or libc).  It's
		 * foolish for us to continue after an EMFILE.  We also
		 * seem to tickle kernel bugs on some platforms which
		 * lead to never-ending loops here.  So it seems best
		 * to just exit in most cases.
		 */
                switch (errno) {
#ifdef EPROTO
		    /* EPROTO on certain older kernels really means
		     * ECONNABORTED, so we need to ignore it for them.
		     * See discussion in new-httpd archives nh.9701
		     * search for EPROTO.
		     *
		     * Also see nh.9603, search for EPROTO:
		     * There is potentially a bug in Solaris 2.x x<6,
		     * and other boxes that implement tcp sockets in
		     * userland (i.e. on top of STREAMS).  On these
		     * systems, EPROTO can actually result in a fatal
		     * loop.  See PR#981 for example.  It's hard to
		     * handle both uses of EPROTO.
		     */
                case EPROTO:
#endif
#ifdef ECONNABORTED
                case ECONNABORTED:
#endif
		    /* Linux generates the rest of these, other tcp
		     * stacks (i.e. bsd) tend to hide them behind
		     * getsockopt() interfaces.  They occur when
		     * the net goes sour or the client disconnects
		     * after the three-way handshake has been done
		     * in the kernel but before userland has picked
		     * up the socket.
		     */
#ifdef ECONNRESET
                case ECONNRESET:
#endif
#ifdef ETIMEDOUT
                case ETIMEDOUT:
#endif
#ifdef EHOSTUNREACH
		case EHOSTUNREACH:
#endif
#ifdef ENETUNREACH
		case ENETUNREACH:
#endif
                    break;

		default:
		    aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
				"accept: (client socket)");
		    clean_child_exit(1);
		}
	    }

	    /* go around again, safe to die */
	    usr1_just_die = 1;
	    if (deferred_die) {
		/* ok maybe not, see ya later */
		clean_child_exit(0);
	    }
	    /* or maybe we missed a signal, you never know on systems
	     * without reliable signals
	     */
	    sync_scoreboard_image();
	    if (scoreboard_image->global.exit_generation >= generation) {
		clean_child_exit(0);
	    }
	}

	SAFE_ACCEPT(accept_mutex_off());	/* unlock after "accept" */

	/* We've got a socket, let's at least process one request off the
	 * socket before we accept a graceful restart request.
	 */
	signal(SIGUSR1, SIG_IGN);

	note_cleanups_for_fd(ptrans, csd);

	/*
	 * We now have a connection, so set it up with the appropriate
	 * socket options, file descriptors, and read/write buffers.
	 */

	clen = sizeof(sa_server);
	if (getsockname(csd, &sa_server, &clen) < 0) {
	    aplog_error(APLOG_MARK, APLOG_ERR, server_conf, "getsockname");
	    continue;
	}

	sock_disable_nagle(csd);

	(void) update_child_status(my_child_num, SERVER_BUSY_READ,
				   (request_rec *) NULL);

	conn_io = bcreate(ptrans, B_RDWR | B_SOCKET);

#ifdef B_SFIO
	(void) sfdisc(conn_io->sf_in, SF_POPDISC);
	sfdisc(conn_io->sf_in, bsfio_new(conn_io->pool, conn_io));
	sfsetbuf(conn_io->sf_in, NULL, 0);

	(void) sfdisc(conn_io->sf_out, SF_POPDISC);
	sfdisc(conn_io->sf_out, bsfio_new(conn_io->pool, conn_io));
	sfsetbuf(conn_io->sf_out, NULL, 0);
#endif

	dupped_csd = csd;
#if defined(NEED_DUPPED_CSD)
	if ((dupped_csd = dup(csd)) < 0) {
	    aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
			"dup: couldn't duplicate csd");
	    dupped_csd = csd;	/* Oh well... */
	}
	note_cleanups_for_fd(ptrans, dupped_csd);
#endif
	bpushfd(conn_io, csd, dupped_csd);

	current_conn = ap__new_connection(ptrans, server_conf, conn_io,
				          (struct sockaddr_in *) &sa_client,
				          (struct sockaddr_in *) &sa_server,
				          my_child_num);

	/*
	 * Read and process each request found on our connection
	 * until no requests are left or we decide to close.
	 */

	while ((r = read_request(current_conn)) != NULL) {

	    /* read_request_line has already done a
	     * signal (SIGUSR1, SIG_IGN);
	     */

	    (void) update_child_status(my_child_num, SERVER_BUSY_WRITE, r);

	    process_request(r);

#if defined(STATUS)
	    increment_counts(my_child_num, r);
#endif

	    if (!current_conn->keepalive || current_conn->aborted)
		break;

	    destroy_pool(r->pool);
	    (void) update_child_status(my_child_num, SERVER_BUSY_KEEPALIVE,
				       (request_rec *) NULL);

	    sync_scoreboard_image();
	    if (scoreboard_image->global.exit_generation >= generation) {
		bclose(conn_io);
		clean_child_exit(0);
	    }

	    /* In case we get a graceful restart while we're blocked
	     * waiting for the request.
	     *
	     * XXX: This isn't perfect, we might actually read the
	     * request and then just die without saying anything to
	     * the client.  This can be fixed by using deferred_die
	     * but you have to teach buff.c about it so that it can handle
	     * the EINTR properly.
	     *
	     * In practice though browsers (have to) expect keepalive
	     * connections to close before receiving a response because
	     * of network latencies and server timeouts.
	     */
	    usr1_just_die = 1;
	    signal(SIGUSR1, usr1_handler);
	}

	/*
	 * Close the connection, being careful to send out whatever is still
	 * in our buffers.  If possible, try to avoid a hard close until the
	 * client has ACKed our FIN and/or has stopped sending us data.
	 */

#ifdef NO_LINGCLOSE
	bclose(conn_io);	/* just close it */
#else
	if (r && r->connection
	    && !r->connection->aborted
	    && r->connection->client
	    && (r->connection->client->fd >= 0)) {

	    lingering_close(r);
	}
	else {
	    bsetflag(conn_io, B_EOUT, 1);
	    bclose(conn_io);
	}
#endif
    }
}

static int make_child(server_rec *s, int slot, time_t now)
{
    int pid;

    if (slot + 1 > max_daemons_limit) {
	max_daemons_limit = slot + 1;
    }

    if (one_process) {
	signal(SIGHUP, just_die);
	signal(SIGTERM, just_die);
	child_main(slot);
    }

    /* avoid starvation */
    head_listener = head_listener->next;

    Explain1("Starting new child in slot %d", slot);
    (void) update_child_status(slot, SERVER_STARTING, (request_rec *) NULL);

    if ((pid = fork()) == -1) {
	aplog_error(APLOG_MARK, APLOG_ERR, s, "fork: Unable to fork new process");

	/* fork didn't succeed. Fix the scoreboard or else
	 * it will say SERVER_STARTING forever and ever
	 */
	(void) update_child_status(slot, SERVER_DEAD, (request_rec *) NULL);

	/* In case system resources are maxxed out, we don't want
	   Apache running away with the CPU trying to fork over and
	   over and over again. */
	sleep(10);

	return -1;
    }

    if (!pid) {
	RAISE_SIGSTOP(MAKE_CHILD);
	MONCONTROL(1);
	/* Disable the restart signal handlers and enable the just_die stuff.
	 * Note that since restart() just notes that a restart has been
	 * requested there's no race condition here.
	 */
	signal(SIGHUP, just_die);
	signal(SIGUSR1, just_die);
	signal(SIGTERM, just_die);
	child_main(slot);
    }

#ifdef OPTIMIZE_TIMEOUTS
    scoreboard_image->parent[slot].last_rtime = now;
#endif
    scoreboard_image->parent[slot].pid = pid;
#ifdef SCOREBOARD_FILE
    lseek(scoreboard_fd, XtOffsetOf(scoreboard, parent[slot]), 0);
    force_write(scoreboard_fd, &scoreboard_image->parent[slot],
		sizeof(parent_score));
#endif

    return 0;
}


/* start up a bunch of children */
static void startup_children(int number_to_start)
{
    int i;
    time_t now = time(0);

    for (i = 0; number_to_start && i < daemons_limit; ++i) {
	if (scoreboard_image->servers[i].status != SERVER_DEAD) {
	    continue;
	}
	if (make_child(server_conf, i, now) < 0) {
	    break;
	}
	--number_to_start;
    }
}


/*
 * idle_spawn_rate is the number of children that will be spawned on the
 * next maintenance cycle if there aren't enough idle servers.  It is
 * doubled up to MAX_SPAWN_RATE, and reset only when a cycle goes by
 * without the need to spawn.
 */
static int idle_spawn_rate = 1;
#ifndef MAX_SPAWN_RATE
#define MAX_SPAWN_RATE	(32)
#endif
static int hold_off_on_exponential_spawning;

static void perform_idle_server_maintenance(void)
{
    int i;
    int to_kill;
    int idle_count;
    short_score *ss;
    time_t now = time(0);
    int free_length;
    int free_slots[MAX_SPAWN_RATE];
    int last_non_dead;
    int total_non_dead;

    /* initialize the free_list */
    free_length = 0;

    to_kill = -1;
    idle_count = 0;
    last_non_dead = -1;
    total_non_dead = 0;

    sync_scoreboard_image();
    for (i = 0; i < daemons_limit; ++i) {
	if (i >= max_daemons_limit && free_length == idle_spawn_rate)
	    break;
	ss = &scoreboard_image->servers[i];
	switch (ss->status) {
	    /* We consider a starting server as idle because we started it
	     * at least a cycle ago, and if it still hasn't finished starting
	     * then we're just going to swamp things worse by forking more.
	     * So we hopefully won't need to fork more if we count it.
	     */
	case SERVER_STARTING:
	case SERVER_READY:
	    ++ idle_count;
	    /* always kill the highest numbered child if we have to...
	     * no really well thought out reason ... other than observing
	     * the server behaviour under linux where lower numbered children
	     * tend to service more hits (and hence are more likely to have
	     * their data in cpu caches).
	     */
	    to_kill = i;
	    break;
	case SERVER_DEAD:
	    /* try to keep children numbers as low as possible */
	    if (free_length < idle_spawn_rate) {
		free_slots[free_length] = i;
		++free_length;
	    }
	    break;
	}
	if (ss->status != SERVER_DEAD) {
	    ++total_non_dead;
	    last_non_dead = i;
#ifdef OPTIMIZE_TIMEOUTS
	    if (ss->timeout_len) {
		/* if it's a live server, with a live timeout then
		 * start checking its timeout */
		parent_score *ps = &scoreboard_image->parent[i];
		if (ss->cur_vtime != ps->last_vtime) {
		    /* it has made progress, so update its last_rtime,
		     * last_vtime */
		    ps->last_rtime = now;
		    ps->last_vtime = ss->cur_vtime;
		}
		else if (ps->last_rtime + ss->timeout_len < now) {
		    /* no progress, and the timeout length has been exceeded */
		    ss->timeout_len = 0;
		    kill(ps->pid, SIGALRM);
		}
	    }
#endif
	}
    }
    max_daemons_limit = last_non_dead + 1;
    if (idle_count > daemons_max_free) {
	/* kill off one child... we use SIGUSR1 because that'll cause it to
	 * shut down gracefully, in case it happened to pick up a request
	 * while we were counting
	 */
	kill(scoreboard_image->parent[to_kill].pid, SIGUSR1);
	idle_spawn_rate = 1;
    }
    else if (idle_count < daemons_min_free) {
	/* terminate the free list */
	if (free_length == 0) {
	    /* only report this condition once */
	    static int reported = 0;

	    if (!reported) {
		aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, server_conf,
			    "server reached MaxClients setting, consider"
			    " raising the MaxClients setting");
		reported = 1;
	    }
	    idle_spawn_rate = 1;
	}
	else {
	    if (idle_spawn_rate >= 8) {
		aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, server_conf,
		    "server seems busy, (you may need "
		    "to increase StartServers, or Min/MaxSpareServers), "
		    "spawning %d children, there are %d idle, and "
		    "%d total children", idle_spawn_rate,
		    idle_count, total_non_dead);
	    }
	    for (i = 0; i < free_length; ++i) {
		make_child(server_conf, free_slots[i], now);
	    }
	    /* the next time around we want to spawn twice as many if this
	     * wasn't good enough, but not if we've just done a graceful
	     */
	    if (hold_off_on_exponential_spawning) {
		--hold_off_on_exponential_spawning;
	    }
	    else if (idle_spawn_rate < MAX_SPAWN_RATE) {
		idle_spawn_rate *= 2;
	    }
	}
    }
    else {
	idle_spawn_rate = 1;
    }
}


/*****************************************************************
 * Executive routines.
 */

#ifndef STANDALONE_MAIN
#define STANDALONE_MAIN standalone_main

void standalone_main(int argc, char **argv)
{
    int remaining_children_to_start;

    standalone = 1;

    is_graceful = 0;
    ++generation;

    if (!one_process) {
	detach();
    }
    else {
	MONCONTROL(1);
    }

    my_pid = getpid();

    do {
	copy_listeners(pconf);
	if (!is_graceful) {
	    restart_time = time(NULL);
	}
#ifdef SCOREBOARD_FILE
	else if (scoreboard_fd != -1) {
	    kill_cleanup(pconf, NULL, cleanup_scoreboard_file);
	    kill_cleanups_for_fd(pconf, scoreboard_fd);
	}
#endif
	clear_pool(pconf);
	ptrans = make_sub_pool(pconf);

	server_conf = read_config(pconf, ptrans, server_confname);
	setup_listeners(pconf);
	open_logs(server_conf, pconf);
	log_pid(pconf, pid_fname);
	init_modules(pconf, server_conf);
	SAFE_ACCEPT(accept_mutex_init(pconf));
	if (!is_graceful) {
	    reinit_scoreboard(pconf);
	}
#ifdef SCOREBOARD_FILE
	else {
	    scoreboard_fname = server_root_relative(pconf, scoreboard_fname);
	    note_cleanups_for_fd(pconf, scoreboard_fd);
	}
#endif

	set_signals();

	if (daemons_max_free < daemons_min_free + 1)	/* Don't thrash... */
	    daemons_max_free = daemons_min_free + 1;

	/* If we're doing a graceful_restart then we're going to see a lot
	 * of children exiting immediately when we get into the main loop
	 * below (because we just sent them SIGUSR1).  This happens pretty
	 * rapidly... and for each one that exits we'll start a new one until
	 * we reach at least daemons_min_free.  But we may be permitted to
	 * start more than that, so we'll just keep track of how many we're
	 * supposed to start up without the 1 second penalty between each fork.
	 */
	remaining_children_to_start = daemons_to_start;
	if (remaining_children_to_start > daemons_limit) {
	    remaining_children_to_start = daemons_limit;
	}
	if (!is_graceful) {
	    startup_children(remaining_children_to_start);
	    remaining_children_to_start = 0;
	}
	else {
	    /* give the system some time to recover before kicking into
	     * exponential mode */
	    hold_off_on_exponential_spawning = 10;
	}

	aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_NOTICE, server_conf,
		    "%s configured -- resuming normal operations",
		    SERVER_VERSION);
	aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, server_conf,
		    "Server built: %s", SERVER_BUILT);
	restart_pending = shutdown_pending = 0;

	while (!restart_pending && !shutdown_pending) {
	    int child_slot;
	    int status;
	    int pid = wait_or_timeout(&status);

	    /* XXX: if it takes longer than 1 second for all our children
	     * to start up and get into IDLE state then we may spawn an
	     * extra child
	     */
	    if (pid >= 0) {
		/* Child died... note that it's gone in the scoreboard. */
		sync_scoreboard_image();
		child_slot = find_child_by_pid(pid);
		Explain2("Reaping child %d slot %d", pid, child_slot);
		if (child_slot >= 0) {
		    (void) update_child_status(child_slot, SERVER_DEAD,
					       (request_rec *) NULL);
		    if (remaining_children_to_start
			&& child_slot < daemons_limit) {
			/* we're still doing a 1-for-1 replacement of dead
			 * children with new children
			 */
			make_child(server_conf, child_slot, time(0));
			--remaining_children_to_start;
		    }
#ifndef NO_OTHER_CHILD
		}
		else if (reap_other_child(pid, status) == 0) {
		    /* handled */
#endif
		}
		else if (is_graceful) {
		    /* Great, we've probably just lost a slot in the
		     * scoreboard.  Somehow we don't know about this
		     * child.
		     */
		    aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_WARNING, server_conf,
				"long lost child came home! (pid %d)", pid);
		}
		/* Don't perform idle maintenance when a child dies,
		 * only do it when there's a timeout.  Remember only a
		 * finite number of children can die, and it's pretty
		 * pathological for a lot to die suddenly.
		 */
		continue;
	    }
	    else if (remaining_children_to_start) {
		/* we hit a 1 second timeout in which none of the previous
		 * generation of children needed to be reaped... so assume
		 * they're all done, and pick up the slack if any is left.
		 */
		startup_children(remaining_children_to_start);
		remaining_children_to_start = 0;
		/* In any event we really shouldn't do the code below because
		 * few of the servers we just started are in the IDLE state
		 * yet, so we'd mistakenly create an extra server.
		 */
		continue;
	    }

	    perform_idle_server_maintenance();
	}

	if (shutdown_pending) {
	    /* Time to gracefully shut down:
	     * Kill child processes, tell them to call child_exit, etc...
	     */
	    if (ap_killpg(pgrp, SIGTERM) < 0) {
		aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "killpg SIGTERM");
	    }
	    reclaim_child_processes(1);		/* Start with SIGTERM */
	    aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_NOTICE, server_conf,
			"httpd: caught SIGTERM, shutting down");

	    clean_parent_exit(0);
	}

	/* we've been told to restart */
	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);

	if (one_process) {
	    /* not worth thinking about */
	    clean_parent_exit(0);
	}

	if (is_graceful) {
#ifndef SCOREBOARD_FILE
	    int i;
#endif

	    /* USE WITH CAUTION:  Graceful restarts are not known to work
	     * in various configurations on the architectures we support. */
	    scoreboard_image->global.exit_generation = generation;
	    update_scoreboard_global();

	    aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_NOTICE, server_conf,
			"SIGUSR1 received.  Doing graceful restart");

	    /* kill off the idle ones */
	    if (ap_killpg(pgrp, SIGUSR1) < 0) {
		aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "killpg SIGUSR1");
	    }
#ifndef SCOREBOARD_FILE
	    /* This is mostly for debugging... so that we know what is still
	     * gracefully dealing with existing request.  But we can't really
	     * do it if we're in a SCOREBOARD_FILE because it'll cause
	     * corruption too easily.
	     */
	    sync_scoreboard_image();
	    for (i = 0; i < daemons_limit; ++i) {
		if (scoreboard_image->servers[i].status != SERVER_DEAD) {
		    scoreboard_image->servers[i].status = SERVER_GRACEFUL;
		}
	    }
#endif
	}
	else {
	    /* Kill 'em off */
	    if (ap_killpg(pgrp, SIGHUP) < 0) {
		aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "killpg SIGHUP");
	    }
	    reclaim_child_processes(0);		/* Not when just starting up */
	    aplog_error(APLOG_MARK, APLOG_NOERRNO|APLOG_NOTICE, server_conf,
			"SIGHUP received.  Attempting to restart");
	}
	++generation;
    } while (restart_pending);

    /*add_common_vars(NULL);*/
}				/* standalone_main */
#else
/* prototype */
void STANDALONE_MAIN(int argc, char **argv);
#endif /* STANDALONE_MAIN */

extern char *optarg;
extern int optind;

int main(int argc, char *argv[])
{
    int c;

    MONCONTROL(0);

#ifdef AUX
    (void) set42sig();
#endif

#ifdef SecureWare
    if (set_auth_parameters(argc, argv) < 0)
	perror("set_auth_parameters");
    if (getluid() < 0)
	if (setluid(getuid()) < 0)
	    perror("setluid");
    if (setreuid(0, 0) < 0)
	perror("setreuid");
#endif

    init_alloc();
    pconf = permanent_pool;
    ptrans = make_sub_pool(pconf);

    pcommands = make_sub_pool(NULL);
    server_pre_read_config  = make_array(pcommands, 1, sizeof(char *));
    server_post_read_config = make_array(pcommands, 1, sizeof(char *));
    
    server_argv0 = argv[0];
    ap_cpystrn(server_root, HTTPD_ROOT, sizeof(server_root));
    ap_cpystrn(server_confname, SERVER_CONFIG_FILE, sizeof(server_confname));

    setup_prelinked_modules();

    while ((c = getopt(argc, argv, "C:c:Xd:f:vVhlZ:")) != -1) {
	char **new;
	switch (c) {
	case 'c':
	    new = (char **)push_array(server_post_read_config);
	    *new = pstrdup(pcommands, optarg);
	    break;
	case 'C':
	    new = (char **)push_array(server_pre_read_config);
	    *new = pstrdup(pcommands, optarg);
	    break;
	case 'd':
	    ap_cpystrn(server_root, optarg, sizeof(server_root));
	    break;
	case 'f':
	    ap_cpystrn(server_confname, optarg, sizeof(server_confname));
	    break;
	case 'v':
	    printf("Server version %s.\n", SERVER_VERSION);
	    printf("Server built:  %s\n", SERVER_BUILT);
	    exit(0);
	case 'V':
	    show_compile_settings();
	    exit(0);
	case 'h':
	    show_directives();
	    exit(0);
	case 'l':
	    show_modules();
	    exit(0);
	case 'X':
	    ++one_process;	/* Weird debugging mode. */
	    break;
#ifdef DEBUG_SIGSTOP
	case 'Z':
	    raise_sigstop_flags = atoi(optarg);
	    break;
#endif
	case '?':
	    usage(argv[0]);
	}
    }

#ifdef __EMX__
    printf("%s \n", SERVER_VERSION);
#endif

    suexec_enabled = init_suexec();
    server_conf = read_config(pconf, ptrans, server_confname);

    child_timeouts = !standalone || one_process;

    if (standalone) {
	open_logs(server_conf, pconf);
	init_modules(pconf, server_conf);
	STANDALONE_MAIN(argc, argv);
    }
    else {
	conn_rec *conn;
	request_rec *r;
	struct sockaddr sa_server, sa_client;
	BUFF *cio;
	NET_SIZE_T l;

	/* Yes this is called twice. */
	init_modules(pconf, server_conf);
	open_logs(server_conf, pconf);
	init_modules(pconf, server_conf);
	set_group_privs();

#ifdef MPE
	/* Only try to switch if we're running as MANAGER.SYS */
	if (geteuid() == 1 && user_id > 1) {
	    GETPRIVMODE();
	    if (setuid(user_id) == -1) {
		GETUSERMODE();
		aplog_error(APLOG_MARK, APLOG_ALERT, server_conf,
			    "setuid: unable to change uid");
		exit(1);
	    }
	    GETUSERMODE();
	}
#else
	/* Only try to switch if we're running as root */
	if (!geteuid() && setuid(user_id) == -1) {
	    aplog_error(APLOG_MARK, APLOG_ALERT, server_conf,
			"setuid: unable to change uid");
	    exit(1);
	}
#endif
	if (ap_setjmp(jmpbuffer)) {
	    exit(0);
	}

	l = sizeof(sa_client);
	if ((getpeername(fileno(stdin), &sa_client, &l)) < 0) {
/* get peername will fail if the input isn't a socket */
	    perror("getpeername");
	    memset(&sa_client, '\0', sizeof(sa_client));
	}

	l = sizeof(sa_server);
	if (getsockname(fileno(stdin), &sa_server, &l) < 0) {
	    perror("getsockname");
	    fprintf(stderr, "Error getting local address\n");
	    exit(1);
	}
	server_conf->port = ntohs(((struct sockaddr_in *) &sa_server)->sin_port);
	cio = bcreate(ptrans, B_RDWR | B_SOCKET);
#ifdef MPE
/* HP MPE 5.5 inetd only passes the incoming socket as stdin (fd 0), whereas
   HPUX inetd passes the incoming socket as stdin (fd 0) and stdout (fd 1).
   Go figure.  SR 5003355016 has been submitted to request that the existing
   functionality be documented, and then to enhance the functionality to be
   like HPUX. */

	cio->fd = fileno(stdin);
#else
	cio->fd = fileno(stdout);
#endif
	cio->fd_in = fileno(stdin);
	conn = ap__new_connection(ptrans, server_conf, cio,
			          (struct sockaddr_in *) &sa_client,
			          (struct sockaddr_in *) &sa_server, -1);
	r = read_request(conn);
	if (r)
	    process_request(r);	/* else premature EOF (ignore) */

	while (r && conn->keepalive && !conn->aborted) {
	    destroy_pool(r->pool);
	    r = read_request(conn);
	    if (r)
		process_request(r);
	}

	bclose(cio);
    }
    exit(0);
}

#ifdef __EMX__
#ifdef USE_MMAP_SCOREBOARD
/* The next two routines are used to access shared memory under OS/2.  */
/* This requires EMX v09c to be installed.                           */

caddr_t create_shared_heap(const char *name, size_t size)
{
    ULONG rc;
    void *mem;
    Heap_t h;

    rc = DosAllocSharedMem(&mem, name, size,
			   PAG_COMMIT | PAG_READ | PAG_WRITE);
    if (rc != 0)
	return NULL;
    h = _ucreate(mem, size, !_BLOCK_CLEAN, _HEAP_REGULAR | _HEAP_SHARED,
		 NULL, NULL);
    if (h == NULL)
	DosFreeMem(mem);
    return (caddr_t) h;
}

caddr_t get_shared_heap(const char *Name)
{

    PVOID BaseAddress;		/* Pointer to the base address of
				   the shared memory object */
    ULONG AttributeFlags;	/* Flags describing characteristics
				   of the shared memory object */
    APIRET rc;			/* Return code */

    /* Request read and write access to */
    /*   the shared memory object       */
    AttributeFlags = PAG_WRITE | PAG_READ;

    rc = DosGetNamedSharedMem(&BaseAddress, Name, AttributeFlags);

    if (rc != 0) {
	printf("DosGetNamedSharedMem error: return code = %ld", rc);
	return 0;
    }

    return BaseAddress;
}
#endif
#endif

#else /* ndef MULTITHREAD */


/**********************************************************************
 * Multithreaded implementation
 *
 * This code is fairly specific to Win32.
 *
 * The model used to handle requests is a set of threads. One "main"
 * thread listens for new requests. When something becomes
 * available, it does a select and places the newly available socket
 * onto a list of "jobs" (add_job()). Then any one of a fixed number
 * of "worker" threads takes the top job off the job list with
 * remove_job() and handles that connection to completion. After
 * the connection has finished the thread is free to take another
 * job from the job list.
 *
 * In the code, the "main" thread is running within the worker_main()
 * function. The first thing this function does is create the
 * worker threads, which operate in the child_sub_main() function. The
 * main thread then goes into a loop within worker_main() where they
 * do a select() on the listening sockets. The select times out once
 * per second so that the thread can check for an "exit" signal
 * from the parent process (see below). If this signal is set, the 
 * thread can exit, but only after it has accepted all incoming
 * connections already in the listen queue (since Win32 appears
 * to through away listened but unaccepted connections when a 
 * process dies).
 *
 * Because the main and worker threads exist within a single process
 * they are vulnerable to crashes or memory leaks (crashes can also
 * be caused within modules, of course). There also needs to be a 
 * mechanism to perform restarts and shutdowns. This is done by
 * creating the main & worker threads within a subprocess. A
 * main process (the "parent process") creates one (or more) 
 * processes to do the work, then the parent sits around waiting
 * for the working process to die, in which case it starts a new
 * one. The parent process also handles restarts (by creating
 * a new working process then signalling the previous working process 
 * exit ) and shutdowns (by signalling the working process to exit).
 * The parent process operates within the master_main() function. This
 * process also handles requests from the service manager (NT only).
 *
 * Signalling between the parent and working process uses a Win32
 * event. Each child has a unique name for the event, which is
 * passed to it with the -c argument when the child is spawned. The
 * parent sets (signals) this event to tell the child to die.
 * At present all children do a graceful die - they finish all
 * current jobs _and_ empty the listen queue before they exit.
 * A non-graceful die would need a second event.
 *
 * The code below starts with functions at the lowest level -
 * worker threads, and works up to the top level - the main()
 * function of the parent process.
 *
 * The scoreboard (in process memory) contains details of the worker
 * threads (within the active working process). There is no shared
 * "scoreboard" between processes, since only one is ever active
 * at once (or at most, two, when one has been told to shutdown but
 * is processes outstanding requests, and a new one has been started).
 * This is controlled by a "start_mutex" which ensures only one working
 * process is active at once.
 **********************************************************************/

/* The code protected by #ifdef UNGRACEFUL_RESTARTS/#endif sections
 * could implement a sort-of ungraceful restart for Win32. instead of
 * graceful restarts. 
 *
 * However it does not work too well because it does not intercept a
 * connection already in progress (in child_sub_main()). We'd have to
 * get that to poll on the exit event. 
 */

/*
 * Definition of jobs, shared by main and worker threads.
 */

typedef struct joblist_s {
    struct joblist_s *next;
    int sock;
} joblist;

/*
 * Globals common to main and worker threads. This structure is not
 * used by the parent process.
 */

typedef struct globals_s {
#ifdef UNGRACEFUL_RESTART
    HANDLE thread_exit_event;
#else
    int exit_now;
#endif
    semaphore *jobsemaphore;
    joblist *jobhead;
    joblist *jobtail;
    mutex *jobmutex;
    int jobcount;
} globals;

globals allowed_globals =
{0, NULL, NULL, NULL, 0};

/*
 * add_job()/remove_job() - add or remove an accepted socket from the
 * list of sockets connected to clients. allowed_globals.jobmutex protects
 * against multiple concurrent access to the linked list of jobs.
 */

void add_job(int sock)
{
    joblist *new_job;

    ap_assert(allowed_globals.jobmutex);
    /* TODO: If too many jobs in queue, sleep, check for problems */
    acquire_mutex(allowed_globals.jobmutex);
    new_job = (joblist *) malloc(sizeof(joblist));
    new_job->next = NULL;
    new_job->sock = sock;
    if (allowed_globals.jobtail != NULL)
	allowed_globals.jobtail->next = new_job;
    allowed_globals.jobtail = new_job;
    if (!allowed_globals.jobhead)
	allowed_globals.jobhead = new_job;
    allowed_globals.jobcount++;
    release_semaphore(allowed_globals.jobsemaphore);
    release_mutex(allowed_globals.jobmutex);
}

int remove_job()
{
    joblist *job;
    int sock;

#ifdef UNGRACEFUL_RESTART
    HANDLE hObjects[2];
    int rv;

    hObjects[0] = allowed_globals.jobsemaphore;
    hObjects[1] = allowed_globals.thread_exit_event;

    rv = WaitForMultipleObjects(2, hObjects, FALSE, INFINITE);
    ap_assert(rv != WAIT_FAILED);
    if (rv == WAIT_OBJECT_0 + 1) {
	/* thread_exit_now */
	APD1("thread got exit now event");
	return -1;
    }
    /* must be semaphore */
#else
    acquire_semaphore(allowed_globals.jobsemaphore);
#endif
    ap_assert(allowed_globals.jobmutex);

#ifdef UNGRACEFUL_RESTART
    if (!allowed_globals.jobhead) {
#else
    acquire_mutex(allowed_globals.jobmutex);
    if (allowed_globals.exit_now && !allowed_globals.jobhead) {
#endif
	release_mutex(allowed_globals.jobmutex);
	return (-1);
    }
    job = allowed_globals.jobhead;
    ap_assert(job);
    allowed_globals.jobhead = job->next;
    if (allowed_globals.jobhead == NULL)
	allowed_globals.jobtail = NULL;
    release_mutex(allowed_globals.jobmutex);
    sock = job->sock;
    free(job);
    return (sock);
}

/*
 * child_sub_main() - this is the main loop for the worker threads
 *
 * Each thread runs within this function. They wait within remove_job()
 * for a job to become available, then handle all the requests on that
 * connection until it is closed, then return to remove_job().
 *
 * The worker thread will exit when it removes a job which contains
 * socket number -1. This provides a graceful thread exit, since
 * it will never exit during a connection.
 *
 * This code in this function is basically equivalent to the child_main()
 * from the multi-process (Unix) environment, except that we
 *
 *  - do not call child_init_modules (child init API phase)
 *  - block in remove_job, and when unblocked we have an already
 *    accepted socket, instead of blocking on a mutex or select().
 */

void child_sub_main(int child_num)
{
    NET_SIZE_T clen;
    struct sockaddr sa_server;
    struct sockaddr sa_client;
    pool *ptrans;
    int requests_this_child = 0;
    int csd = -1;
    int dupped_csd = -1;
    int srv = 0;

    ptrans = make_sub_pool(pconf);

    (void) update_child_status(child_num, SERVER_READY, (request_rec *) NULL);

    /*
     * Setup the jump buffers so that we can return here after
     * a signal or a timeout (yeah, I know, same thing).
     */
#if defined(USE_LONGJMP)
    setjmp(jmpbuffer);
#else
    sigsetjmp(jmpbuffer, 1);
#endif
#ifdef SIGURG
    signal(SIGURG, timeout);
#endif

    while (1) {
	BUFF *conn_io;
	request_rec *r;

	/*
	 * (Re)initialize this child to a pre-connection state.
	 */

	set_callback_and_alarm(NULL, 0);	/* Cancel any outstanding alarms. */
	timeout_req = NULL;	/* No request in progress */
	current_conn = NULL;
#ifdef SIGPIPE
	signal(SIGPIPE, timeout);
#endif

	clear_pool(ptrans);

	(void) update_child_status(child_num, SERVER_READY, (request_rec *) NULL);

	/* Get job from the job list. This will block until a job is ready.
	 * If -1 is returned then the main thread wants us to exit.
	 */
	csd = remove_job();
	if (csd == -1)
	    break;		/* time to exit */
	requests_this_child++;

	note_cleanups_for_socket(ptrans, csd);

	/*
	 * We now have a connection, so set it up with the appropriate
	 * socket options, file descriptors, and read/write buffers.
	 */

	clen = sizeof(sa_server);
	if (getsockname(csd, &sa_server, &clen) < 0) {
	    aplog_error(APLOG_MARK, APLOG_WARNING, server_conf, "getsockname");
	    continue;
	}
	clen = sizeof(sa_client);
	if ((getpeername(csd, &sa_client, &clen)) < 0) {
	    /* get peername will fail if the input isn't a socket */
	    perror("getpeername");
	    memset(&sa_client, '\0', sizeof(sa_client));
	}

	sock_disable_nagle(csd);

	(void) update_child_status(child_num, SERVER_BUSY_READ,
				   (request_rec *) NULL);

	conn_io = bcreate(ptrans, B_RDWR | B_SOCKET);
	dupped_csd = csd;
#if defined(NEED_DUPPED_CSD)
	if ((dupped_csd = dup(csd)) < 0) {
	    aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
			"dup: couldn't duplicate csd");
	    dupped_csd = csd;	/* Oh well... */
	}
	note_cleanups_for_socket(ptrans, dupped_csd);
#endif
	bpushfd(conn_io, csd, dupped_csd);

	current_conn = ap__new_connection(ptrans, server_conf, conn_io,
				          (struct sockaddr_in *) &sa_client,
				          (struct sockaddr_in *) &sa_server,
				          child_num);

	/*
	 * Read and process each request found on our connection
	 * until no requests are left or we decide to close.
	 */

	while ((r = read_request(current_conn)) != NULL) {
	    (void) update_child_status(child_num, SERVER_BUSY_WRITE, r);

	    process_request(r);

#if defined(STATUS)
	    increment_counts(child_num, r);
#endif

	    if (!current_conn->keepalive || current_conn->aborted)
		break;

	    destroy_pool(r->pool);
	    (void) update_child_status(child_num, SERVER_BUSY_KEEPALIVE,
				       (request_rec *) NULL);

	    sync_scoreboard_image();
	}

	/*
	 * Close the connection, being careful to send out whatever is still
	 * in our buffers.  If possible, try to avoid a hard close until the
	 * client has ACKed our FIN and/or has stopped sending us data.
	 */
	kill_cleanups_for_socket(ptrans, csd);

#ifdef NO_LINGCLOSE
	bclose(conn_io);	/* just close it */
#else
	if (r && r->connection
	    && !r->connection->aborted
	    && r->connection->client
	    && (r->connection->client->fd >= 0)) {

	    lingering_close(r);
	}
	else {
	    bsetflag(conn_io, B_EOUT, 1);
	    bclose(conn_io);
	}
#endif
    }
    destroy_pool(ptrans);
    (void) update_child_status(child_num, SERVER_DEAD, NULL);
}


void child_main(int child_num_arg)
{
    /*
     * Only reason for this function, is to pass in
     * arguments to child_sub_main() on its stack so
     * that longjump doesn't try to corrupt its local
     * variables and I don't need to make those
     * damn variables static/global
     */
    child_sub_main(child_num_arg);
}



void cleanup_thread(thread **handles, int *thread_cnt, int thread_to_clean)
{
    int i;

    free_thread(handles[thread_to_clean]);
    for (i = thread_to_clean; i < ((*thread_cnt) - 1); i++)
	handles[i] = handles[i + 1];
    (*thread_cnt)--;
}

/*****************************************************************
 * Executive routines.
 */

extern void main_control_server(void *); /* in hellop.c */

event *exit_event;
mutex *start_mutex;

#define MAX_SELECT_ERRORS 100

void worker_main()
{
    /*
     * I am writing this stuff specifically for NT.
     * have pulled out a lot of the restart and
     * graceful restart stuff, because that is only
     * useful on Unix (not sure it even makes sense
     * in a multi-threaded env.
     */
    int nthreads;
    fd_set main_fds;
    int srv;
    int clen;
    int csd;
    struct sockaddr_in sa_client;
    int total_jobs = 0;
    thread **child_handles;
    int rv;
    time_t end_time;
    int i;
    struct timeval tv;
    int wait_time = 1;
    int start_exit = 0;
    int start_mutex_released = 0;
    int max_jobs_per_exe;
    int max_jobs_after_exit_request;
    HANDLE hObjects[2];
    int count_select_errors = 0;
    pool *pchild;

    pchild = make_sub_pool(pconf);

    standalone = 1;
    sd = -1;
    nthreads = threads_per_child;
    max_jobs_after_exit_request = excess_requests_per_child;
    max_jobs_per_exe = max_requests_per_child;
    if (nthreads <= 0)
	nthreads = 40;
    if (max_jobs_per_exe <= 0)
	max_jobs_per_exe = 0;
    if (max_jobs_after_exit_request <= 0)
	max_jobs_after_exit_request = max_jobs_per_exe / 10;

    if (!one_process)
	detach();

    my_pid = getpid();

    ++generation;

    copy_listeners(pconf);
    restart_time = time(NULL);

    reinit_scoreboard(pconf);

    //acquire_mutex(start_mutex);
    hObjects[0] = (HANDLE)start_mutex;
    hObjects[1] = (HANDLE)exit_event;
    rv = WaitForMultipleObjects(2, hObjects, FALSE, INFINITE);
    if (rv == WAIT_FAILED) {
	aplog_error(APLOG_MARK,APLOG_ERR|APLOG_WIN32ERROR, server_conf,
	    "Waiting for start_mutex or exit_event -- process will exit");

	child_exit_modules(pconf, server_conf);
	destroy_pool(pchild);

	cleanup_scoreboard();
	exit(0);
    }
    if (rv == WAIT_OBJECT_0 + 1) {
	/* exit event signalled - exit now */
	child_exit_modules(pconf, server_conf);
	destroy_pool(pchild);

	cleanup_scoreboard();
	exit(0);
    }
    /* start_mutex obtained, continue into the select() loop */

    setup_listeners(pconf);
    set_signals();

    /*
     * - Initialize allowed_globals
     * - Create the thread table
     * - Spawn off threads
     * - Create listen socket set (done above)
     * - loop {
     *       wait for request
     *       create new job
     *   } while (!time to exit)
     * - Close all listeners
     * - Wait for all threads to complete
     * - Exit
     */

    child_init_modules(pconf, server_conf);

    allowed_globals.jobsemaphore = create_semaphore(0);
    allowed_globals.jobmutex = create_mutex(NULL);

    /* spawn off the threads */
    child_handles = (thread *) alloca(nthreads * sizeof(int));
    {
	int i;

	for (i = 0; i < nthreads; i++) {
	    child_handles[i] = create_thread((void (*)(void *)) child_main, (void *) i);
	}
	if (nthreads > max_daemons_limit) {
	    max_daemons_limit = nthreads;
	}
    }

    while (1) {
#if SEVERELY_VERBOSE
	APD4("child PID %d: thread_main total_jobs=%d start_exit=%d",
		my_pid, total_jobs, start_exit);
#endif
	if ((max_jobs_per_exe && (total_jobs > max_jobs_per_exe) && !start_exit)) {
	    start_exit = 1;
	    wait_time = 1;
	    release_mutex(start_mutex);
	    start_mutex_released = 1;
	    APD2("process PID %d: start mutex released\n", my_pid);
	}
	if (!start_exit) {
	    rv = WaitForSingleObject(exit_event, 0);
	    ap_assert((rv == WAIT_TIMEOUT) || (rv == WAIT_OBJECT_0));
	    if (rv == WAIT_OBJECT_0) {
		APD1("child: exit event signalled, exiting");
		start_exit = 1;
		/* Lets not break yet - we may have threads to clean up */
		/* break;*/
	    }
	    rv = WaitForMultipleObjects(nthreads, child_handles, 0, 0);
	    ap_assert(rv != WAIT_FAILED);
	    if (rv != WAIT_TIMEOUT) {
		rv = rv - WAIT_OBJECT_0;
		ap_assert((rv >= 0) && (rv < nthreads));
		cleanup_thread(child_handles, &nthreads, rv);
		break;
	    }
	}

#if 0
	/* Um, this made us exit before all the connections in our
	 * listen queue were dealt with. 
	 */
	if (start_exit && max_jobs_after_exit_request && (count_down-- < 0))
	    break;
#endif
	tv.tv_sec = wait_time;
	tv.tv_usec = 0;

	memcpy(&main_fds, &listenfds, sizeof(fd_set));
	srv = ap_select(listenmaxfd + 1, &main_fds, NULL, NULL, &tv);
#ifdef WIN32
	if (srv == SOCKET_ERROR) {
	    /* Map the Win32 error into a standard Unix error condition */
	    errno = WSAGetLastError();
	    srv = -1;
	}
#endif /* WIN32 */

	if (srv < 0) {
	    /* Error occurred - if EINTR, loop around with problem */
	    if (errno != EINTR) {
		/* A "real" error occurred, log it and increment the count of
		 * select errors. This count is used to ensure we don't go into
		 * a busy loop of continuour errors.
		 */
		aplog_error(APLOG_MARK, APLOG_ERR, server_conf, "select: (listen)");
		count_select_errors++;
		if (count_select_errors > MAX_SELECT_ERRORS) {
		    aplog_error(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, server_conf,
			"Too many errors in select loop. Child process exiting.");
		    break;
		}
	    }
	    continue;
	}
	count_select_errors = 0;    /* reset count of errors */
	if (srv == 0) {
	    if (start_exit)
		break;
	    else
		continue;
	}

	{
	    listen_rec *lr;

	    lr = find_ready_listener(&main_fds);
	    if (lr != NULL) {
		sd = lr->fd;
	    }
	}
	do {
	    clen = sizeof(sa_client);
	    csd = accept(sd, (struct sockaddr *) &sa_client, &clen);
#ifdef WIN32
	    if (csd == INVALID_SOCKET) {
		csd = -1;
		errno = WSAGetLastError();
	    }
#endif /* WIN32 */
	} while (csd < 0 && errno == EINTR);

	if (csd < 0) {
#if defined(EPROTO) && defined(ECONNABORTED)
	    if ((errno != EPROTO) && (errno != ECONNABORTED))
#elif defined(EPROTO)
	    if (errno != EPROTO)
#elif defined(ECONNABORTED)
	    if (errno != ECONNABORTED)
#endif
		aplog_error(APLOG_MARK, APLOG_ERR, server_conf,
			    "accept: (client socket)");
	}
	else {
	    add_job(csd);
	    total_jobs++;
	}
    }
      
    APD2("process PID %d exiting", my_pid);

    /* Get ready to shutdown and exit */
    allowed_globals.exit_now = 1;
    if (!start_mutex_released) {
	release_mutex(start_mutex);
    }

#ifdef UNGRACEFUL_RESTART
    SetEvent(allowed_globals.thread_exit_event);
#else
    for (i = 0; i < nthreads; i++) {
	add_job(-1);
    }
#endif

    APD2("process PID %d waiting for worker threads to exit", my_pid);
    /* Wait for all your children */
    end_time = time(NULL) + 180;
    while (nthreads) {
	rv = WaitForMultipleObjects(nthreads, child_handles, 0, (end_time - time(NULL)) * 1000);
	if (rv != WAIT_TIMEOUT) {
	    rv = rv - WAIT_OBJECT_0;
	    ap_assert((rv >= 0) && (rv < nthreads));
	    cleanup_thread(child_handles, &nthreads, rv);
	    continue;
	}
	break;
    }

    APD2("process PID %d killing remaining worker threads", my_pid);
    for (i = 0; i < nthreads; i++) {
	kill_thread(child_handles[i]);
	free_thread(child_handles[i]);
    }
#ifdef UNGRACEFUL_RESTART
    ap_assert(CloseHandle(allowed_globals.thread_exit_event));
#endif
    destroy_semaphore(allowed_globals.jobsemaphore);
    destroy_mutex(allowed_globals.jobmutex);

    child_exit_modules(pconf, server_conf);
    destroy_pool(pchild);

    cleanup_scoreboard();

    APD2("process PID %d exited", my_pid);
    exit(0);
}				/* standalone_main */


int create_event_and_spawn(int argc, char **argv, event **ev, int *child_num, char *prefix)
{
    int pid = getpid();
    char buf[40], mod[200];
    int i, rv;
    char **pass_argv = (char **) alloca(sizeof(char *) * (argc + 3));

    sprintf(buf, "%s_%d", prefix, ++(*child_num));
    _flushall();
    *ev = CreateEvent(NULL, TRUE, FALSE, buf);
    APD2("create_event_and_spawn(): created process kill event %s", buf);

    ap_assert(*ev);
    pass_argv[0] = argv[0];
    pass_argv[1] = "-Z";
    pass_argv[2] = buf;
    for (i = 1; i < argc; i++) {
	pass_argv[i + 2] = argv[i];
    }
    pass_argv[argc + 2] = NULL;


    GetModuleFileName(NULL, mod, 200);
    rv = spawnv(_P_NOWAIT, mod, pass_argv);

    return (rv);
}

/**********************************************************************
 * master_main - this is the parent (main) process. We create a
 * child process to do the work, then sit around waiting for either
 * the child to exit, or a restart or exit signal. If the child dies,
 * we just respawn a new one. If we have a shutdown or graceful restart,
 * tell the child to die when it is ready. If it is a non-graceful
 * restart, force the child to die immediately.
 **********************************************************************/

#define MAX_PROCESSES 50 /* must be < MAX_WAIT_OBJECTS-1 */

void cleanup_process(HANDLE *handles, HANDLE *events, int position, int *processes)
{
    int i;
    int handle = 0;

    CloseHandle(handles[position]);
    CloseHandle(events[position]);

    handle = (int)handles[position];

    for (i = position; i < (*processes)-1; i++) {
	handles[i] = handles[i + 1];
	events[i] = events[i + 1];
    }
    (*processes)--;

    APD4("cleanup_processes: removed child in slot %d handle %d, max=%d", position, handle, *processes);
}

void create_process(HANDLE *handles, HANDLE *events, int *processes, int *child_num, char *kill_event_name, int argc, char **argv)
{
    int i = *processes;
    HANDLE kill_event;

    handles[i] = (HANDLE)create_event_and_spawn(argc, argv, &kill_event, child_num, kill_event_name);
    ap_assert(handles[i] >= 0);
    events[i] = kill_event;
    (*processes)++;

    APD4("create_processes: created child in slot %d handle %d, max=%d", 
	(*processes)-1, handles[(*processes)-1], *processes);
}

int master_main(int argc, char **argv)
{
    int nchild = daemons_to_start;
    event **ev;
    int *child;
    int child_num = 0;
    int rv, cld;
    char buf[100];
    int i;
    time_t tmstart;
    HANDLE signal_event;	/* used to signal shutdown/restart to parent */
    HANDLE process_handles[MAX_PROCESSES];
    HANDLE process_kill_events[MAX_PROCESSES];
    int current_live_processes = 0; /* number of child process we know about */
    int processes_to_create = 0;    /* number of child processes to create */
    pool *pparent;  /* pool for the parent process. Cleaned on each restart */

    nchild = 1;	    /* only allowed one child process for current generation */
    processes_to_create = nchild;

    is_graceful = 0;
    ++generation;

    signal_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, "apache-signal");
    if (!signal_event) {
	aplog_error(APLOG_MARK, APLOG_EMERG|APLOG_WIN32ERROR, server_conf,
		    "Cannot open apache-signal event");
	exit(1);
    }

    sprintf(buf, "Apache%d", getpid());
    start_mutex = create_mutex(buf);
    ev = (event **) alloca(sizeof(event *) * nchild);
    child = (int *) alloca(sizeof(int) * (nchild+1));
    while (processes_to_create--) {
	service_set_status(SERVICE_START_PENDING);
	create_process(process_handles, process_kill_events, &current_live_processes, &child_num, buf, argc, argv);
    }

    service_set_status(SERVICE_RUNNING);

    restart_pending = shutdown_pending = 0;

    do { /* restart-pending */
	if (!is_graceful) {
	    restart_time = time(NULL);
	}
	clear_pool(pconf);
	pparent = make_sub_pool(pconf);

	server_conf = read_config(pconf, pparent, server_confname);
	open_logs(server_conf, pconf);
	init_modules(pconf, server_conf);
	if (!is_graceful)
	    reinit_scoreboard(pconf);

	restart_pending = shutdown_pending = 0;

	/* Wait for either a child process to die, or for the stop_event
	 * to be signalled by the service manager or rpc server */
	while (1) {
	    /* Next line will block forever until either a child dies, or we
	     * get signalled on the "apache-signal" event (e.g. if the user is
	     * requesting a shutdown/restart)
	     */
	    if (current_live_processes == 0) {
		/* Shouldn't happen, but better safe than sorry */
		aplog_error(APLOG_MARK,APLOG_ERR|APLOG_NOERRNO, server_conf,
 			"master_main: no child processes alive! creating one");
		create_process(process_handles, process_kill_events, &current_live_processes, &child_num, buf, argc, argv);
		if (processes_to_create) {
		    processes_to_create--;
		}
	    }
	    process_handles[current_live_processes] = signal_event;
	    rv = WaitForMultipleObjects(current_live_processes+1, (HANDLE *)process_handles, 
			FALSE, INFINITE);
	    if (rv == WAIT_FAILED) {
		/* Something serious is wrong */
		aplog_error(APLOG_MARK,APLOG_CRIT|APLOG_WIN32ERROR, server_conf,
		    "WaitForMultipeObjects on process handles and apache-signal -- doing shutdown");
		shutdown_pending = 1;
		break;
	    }
	    ap_assert(rv != WAIT_TIMEOUT);
	    cld = rv - WAIT_OBJECT_0;
	    APD4("main process: wait finished, cld=%d handle %d (max=%d)", cld, process_handles[cld], current_live_processes);
	    if (cld == current_live_processes) {
		/* stop_event is signalled, we should exit now */
		if (ResetEvent(signal_event) == 0)
		    APD1("main process: *** ERROR: ResetEvent(stop_event) failed ***");
		APD3("main process: stop_event signalled: shutdown_pending=%d, restart_pending=%d",
		    shutdown_pending, restart_pending);
		break;
	    }
	    ap_assert(cld < current_live_processes);
	    cleanup_process(process_handles, process_kill_events, cld, &current_live_processes);
	    APD2("main_process: child in slot %d died", rv);
	    if (processes_to_create) {
		create_process(process_handles, process_kill_events, &current_live_processes, &child_num, buf, argc, argv);
		processes_to_create--;
	    }
	}
	if (!shutdown_pending && !restart_pending) {
	    aplog_error(APLOG_MARK,APLOG_CRIT|APLOG_NOERRNO, server_conf,
		"master_main: no shutdown or restart variables set -- doing shutdown");
	    shutdown_pending = 1;
	}
	if (shutdown_pending) {
	    /* tell all child processes to die */
	    for (i = 0; i < current_live_processes; i++) {
		APD3("main process: signalling child #%d handle %d to die", i, process_handles[i]);
		if (SetEvent(process_kill_events[i]) == 0)
		    aplog_error(APLOG_MARK,APLOG_WIN32ERROR, server_conf,
			"SetEvent for child process in slot #%d", i);
	    }

	    break;
	}
	if (restart_pending) {
	    int children_to_kill = current_live_processes;

	    APD1("--- Doing graceful restart ---");

	    processes_to_create = nchild;
	    for (i = 0; i < nchild; ++i) {
		if (current_live_processes >= MAX_PROCESSES)
		    break;
		create_process(process_handles, process_kill_events, &current_live_processes, &child_num, buf, argc, argv);
		processes_to_create--;
	    }
	    for (i = 0; i < children_to_kill; i++) {
		APD3("main process: signalling child #%d handle %d to die", i, process_handles[i]);
		if (SetEvent(process_kill_events[i]) == 0)
		    aplog_error(APLOG_MARK,APLOG_WIN32ERROR, server_conf,
 			"SetEvent for child process in slot #%d", i);
	    }
	}
	++generation;
    } while (restart_pending);

    /* If we dropped out of the loop we definitly want to die completely. We need to
     * make sure we wait for all the child process to exit first.
     */

    APD2("*** main process shutdown, processes=%d ***", current_live_processes);

    CloseHandle(signal_event);

    tmstart = time(NULL);
    while (current_live_processes && ((tmstart+60) > time(NULL))) {
	service_set_status(SERVICE_STOP_PENDING);
	rv = WaitForMultipleObjects(current_live_processes, (HANDLE *)process_handles, FALSE, 2000);
	if (rv == WAIT_TIMEOUT)
	    continue;
	ap_assert(rv != WAIT_FAILED);
	cld = rv - WAIT_OBJECT_0;
	ap_assert(rv < current_live_processes);
	APD4("main_process: child in #%d handle %d died, left=%d", 
	    rv, process_handles[rv], current_live_processes);
	cleanup_process(process_handles, process_kill_events, cld, &current_live_processes);
    }
    for (i = 0; i < current_live_processes; i++) {
	aplog_error(APLOG_MARK,APLOG_ERR|APLOG_NOERRNO, server_conf,
 	    "forcing termination of child #%d (handle %d)", i, process_handles[i]);
	TerminateProcess((HANDLE) process_handles[i], 1);
    }
    service_set_status(SERVICE_STOPPED);

    destroy_pool(pparent);

    destroy_mutex(start_mutex);
    return (0);
}

#ifdef WIN32
__declspec(dllexport)
     int apache_main(int argc, char *argv[])
#else
int main(int argc, char *argv[]) 
#endif
{
    int c;
    int child = 0;
    char *cp;
    int run_as_service = 1;
    int install = 0;

#ifdef AUX
    (void) set42sig();
#endif

#ifdef SecureWare
    if (set_auth_parameters(argc, argv) < 0)
	perror("set_auth_parameters");
    if (getluid() < 0)
	if (setluid(getuid()) < 0)
	    perror("setluid");
    if (setreuid(0, 0) < 0)
	perror("setreuid");
#endif

#ifdef WIN32
    /* Initialize the stupid sockets */
    AMCSocketInitialize();
#endif /* WIN32 */

    init_alloc();
    pconf = permanent_pool;
    ptrans = make_sub_pool(pconf);

    pcommands = make_sub_pool(NULL);
    server_pre_read_config  = make_array(pcommands, 1, sizeof(char *));
    server_post_read_config = make_array(pcommands, 1, sizeof(char *));
    
    server_argv0 = argv[0];
    ap_cpystrn(server_root, HTTPD_ROOT, sizeof(server_root));
    ap_cpystrn(server_confname, SERVER_CONFIG_FILE, sizeof(server_confname));

    setup_prelinked_modules();

    while ((c = getopt(argc, argv, "C:c:Xd:f:vVhlZ:ius")) != -1) {
        char **new;
	switch (c) {
	case 'c':
	    new = (char **)push_array(server_post_read_config);
	    *new = pstrdup(pcommands, optarg);
	    break;
	case 'C':
	    new = (char **)push_array(server_pre_read_config);
	    *new = pstrdup(pcommands, optarg);
	    break;
#ifdef WIN32
	case 'Z':
	    exit_event = open_event(optarg);
	    APD2("child: opened process event %s", optarg);
	    cp = strchr(optarg, '_');
	    ap_assert(cp);
	    *cp = 0;
	    start_mutex = open_mutex(optarg);
	    child = 1;
	    break;
	case 'i':
	    install = 1;
	    break;
	case 'u':
	    install = -1;
	    break;
	case 's':
	    run_as_service = 0;
	    break;
#endif /* WIN32 */
	case 'd':
	    ap_cpystrn(server_root, optarg, sizeof(server_root));
	    break;
	case 'f':
	    ap_cpystrn(server_confname, optarg, sizeof(server_confname));
	    break;
	case 'v':
	    printf("Server version %s.\n", SERVER_VERSION);
	    printf("Server built:  %s\n", SERVER_BUILT);
	    exit(0);
	case 'V':
	    show_compile_settings();
	    exit(0);
	case 'h':
	    show_directives();
	    exit(0);
	case 'l':
	    show_modules();
	    exit(0);
	case 'X':
	    ++one_process;	/* Weird debugging mode. */
	    break;
	case '?':
	    usage(argv[0]);
	}
    }

#ifdef __EMX__
    printf("%s \n", SERVER_VERSION);
#endif
#ifdef WIN32
    if (!child) {
	printf("%s \n", SERVER_VERSION);
    }
#endif
    if (!child && run_as_service) {
	service_cd();
    }

    server_conf = read_config(pconf, ptrans, server_confname);
    log_pid(pconf, pid_fname);
    init_modules(pconf, server_conf);
    suexec_enabled = init_suexec();
    open_logs(server_conf, pconf);
    set_group_privs();

    if (one_process && !exit_event)
	exit_event = create_event(0, 0, NULL);
    if (one_process && !start_mutex)
	start_mutex = create_mutex(NULL);
    /*
     * In the future, the main will spawn off a couple
     * of children and monitor them. As soon as a child
     * exits, it spawns off a new one
     */
    if (child || one_process) {
	if (!exit_event || !start_mutex)
	    exit(-1);
	worker_main();
	destroy_mutex(start_mutex);
	destroy_event(exit_event);
    }
    else {
	service_main(master_main, argc, argv,
			"Apache", install, run_as_service);
    }

    return (0);
}


#endif /* ndef MULTITHREAD */
