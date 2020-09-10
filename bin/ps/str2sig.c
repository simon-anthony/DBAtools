#ifdef __SUN__
#pragma ident "$Header$
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

typedef struct signame {
#ifdef __STDC__
	const char *sigstr;
	const int	signum;
#else
	char *sigstr;
	int	signum;
#endif
} signame_t;

static signame_t signames[] = {
	{ "EXIT",     0 },
	{ "HUP",      SIGHUP },		/* 1	 hangup */
	{ "INT",      SIGINT },		/* 2	 Interrupt */
	{ "QUIT",     SIGQUIT },	/* 3	 quit */
	{ "ILL",      SIGILL },		/* 4	 Illegal instruction (not reset when caught) */
	{ "TRAP",     SIGTRAP },	/* 5	 trace trap (not reset when caught) */
	{ "IOT",      SIGIOT },		/* 6	 Process abort signal */
	{ "EMT",      SIGEMT },		/* 7	 EMT instruction */
	{ "FPE",      SIGFPE },		/* 8	 Floating point exception */
	{ "KILL",     SIGKILL },	/* 9	 kill (cannot be caught of ignored) */
	{ "BUS",      SIGBUS },		/* 10	 bus error */
	{ "SEGV",     SIGSEGV },	/* 11	 Segmentation violation */
	{ "SYS",      SIGSYS },		/* 12	 bad argument to system call */
	{ "PIPE",     SIGPIPE },	/* 13	 write on a pipe with no one to read it */
	{ "ALRM",     SIGALRM },	/* 14	 alarm clock */
	{ "TERM",     SIGTERM },	/* 15	 Software termination signal from kill */
	{ "USR1",     SIGUSR1 },	/* 16	 user defined signal 1 */
	{ "USR2",     SIGUSR2 },	/* 17	 user defined signal 2 */
	{ "CHLD",     SIGCHLD },	/* 18	 Child process terminated or stopped */
	{ "PWR",      SIGPWR },		/* 19	 power state indication */
	{ "VTALRM",   SIGVTALRM },	/* 20 	 virtual timer alarm */
	{ "PROF",     SIGPROF },	/* 21	 profiling timer alarm */
	{ "POLL",     SIGPOLL },	/* 22	 asynchronous I/O */
	{ "WINCH",    SIGWINCH },	/* 23 	 window size change signal */
	{ "STOP",     SIGSTOP },	/* 24	 Stop signal (cannot be caught or ignored) */
	{ "TSTP",     SIGTSTP },	/* 25	 Interactive stop signal */
	{ "CONT",     SIGCONT },	/* 26	 Continue if stopped */
	{ "TTIN",     SIGTTIN },	/* 27	 Read from control terminal attempted by a member of a background process group */
	{ "TTOU",     SIGTTOU },	/* 28	 Write to control terminal attempted by a member of a background process group */
	{ "URG",      SIGURG },		/* 29	 urgent condition on IO channel */
	{ "LOST",     SIGLOST },	/* 30       remote lock lost  (NFS)        */
#ifdef _HPUX_SOURCE
	{ "RESERVED", _SIGRESERVE },		
#endif
#ifdef SIGDIL
	{ "SIGDIL",   SIGDIL },
#elif _HPUX_SOURCE
	{ "SIGOBSOLETE32",   SIGOBSOLETE32 },	/* 32	 SIGDIL 32 is obsolete */
#endif
	{ "XCPU",     SIGXCPU },	/* 33	 CPU time limit exceeded (setrlimit)  */
	{ "XFSZ",     SIGXFSZ },	/* 34	 File size limit exceeded (setrlimit) */
#ifdef _HPUX_SOURCE
	{ "CANCEL",   SIGCANCEL },	/* 35  obsolete:      Used for pthread cancellation. */
#endif
#ifdef _HPUX_SOURCE
	{ "FAULT",    SIGGFAULT },	/* 36  obsolete:      Graphics framebuffer fault */
#endif
#ifdef _HPUX_SOURCE
	{ "RTMIN",    _SIGRTMIN },	/* 37       First (highest priority) realtime signal */
	{ "RTMIN+1",  _SIGRTMIN+1 },
	{ "RTMIN+2",  _SIGRTMIN+2 },
	{ "RTMIN+3",  _SIGRTMIN+3 },
	{ "RTMAX-3",  _SIGRTMAX-3 },
	{ "RTMAX-2",  _SIGRTMAX-2 },
	{ "RTMAX-1",  _SIGRTMAX-1 },
	{ "RTMAX",    _SIGRTMAX }	/* 44       Last (lowest priority) realtime signal */
#endif
};

#define SIGCNT	(sizeof (signames) / sizeof (struct signame ))

static int
#ifdef __STDC__
str2long(const char *p, long *val)
#else
str2long(p, val)
 char *p;
 long *val;
#endif
{
	char *q;
	int error;
	int saved_errno = errno;

	errno = 0;
	*val = strtol(p, &q, 10);

	error = ((errno != 0 || q == p || *q != '\0') ? -1 : 0 );
	errno = saved_errno;

	return (error);
}

int
#ifdef __STDC__
str2sig(const char *s, int *sigp)
#else
str2sig(s, sigp)
 char *s;
 int *sigp;
#endif
{
#ifdef __STDC
	const struct signame *sp;
#else
	struct signame *sp;
#endif

	if (*s >= '0' && *s <= '9') {
		long val;

		if (str2long(s, &val) == -1)
			return (-1);

		for (sp = signames; sp < &signames[SIGCNT]; sp++) {
			if (sp->signum == val) {
				*sigp = sp->signum;
				return (0);
			}
		}
		return (-1);
	} else {
		for (sp = signames; sp < &signames[SIGCNT]; sp++) {
			if (strcmp(sp->sigstr, s) == 0) {
				*sigp = sp->signum;
				return (0);
			}
		}
		return (-1);
	}
}

int
#ifdef __STDC
sig2str(int i, char *s) 
#else
sig2str(i, s) 
 int i;
 char *s; 
#endif
{
#ifdef __STDC
	const struct signame *sp;
#else
	struct signame *sp;
#endif

	for (sp = signames; sp < &signames[SIGCNT]; sp++) {
		if (sp->signum == i) {
			(void) strcpy(s, sp->sigstr);
			return (0);
		}
	}
	return (-1);
}
