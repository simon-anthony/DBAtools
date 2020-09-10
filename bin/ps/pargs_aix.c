#ifdef __SUN__
#pragma ident "$Header: DBAtools/trunk/bin/ps/pargs_aix.c 174 2017-10-25 16:23:50Z xanthos $"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header: DBAtools/trunk/bin/ps/pargs_aix.c 174 2017-10-25 16:23:50Z xanthos $")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header: DBAtools/trunk/bin/ps/pargs_aix.c 174 2017-10-25 16:23:50Z xanthos $";
#endif

/* pargs
 *   Minimal implementation of pargs(1) for AIX based on Solaris.
 *   Not all options supported, but the most useful ones are there.
 *   Simon Anthony
 */

#define PARGS_USAGE "[-aceFlxr] pid"

#define PARGS_OPTS "aceFlxr"
#define UNLIMITED(u) (!((RLIM_INFINITY & u) != RLIM_INFINITY))

#include <sys/param.h>
#include <procinfo.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/cred.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#define min(a, b)      ((a) > (b) ? (b) : (a))
#define max(a, b)      ((a) < (b) ? (b) : (a))

#define COUNT 12

static char    *prog;

static int aflg = 0, cflg = 0, eflg = 0, Fflg = 0, lflg = 0, xflg = 0, rflg = 0,
		   errflg = 0;

static struct procentry64 P; /* process */
static char buf[ARG_MAX];	 /* env vars list */
static char args_buf[ARG_MAX]; /* for getargs() output */

static void printarglist(const char *, const char);
static void printrlimits(struct procentry64 *, int);


int
main(int argc, char *argv[])
{
	extern  char *optarg;
	extern  int optind, opterr;
	char	*optstr;
	char    *options, *value;
	int		c;
	char    *arg, *args;
	pid_t   pid, idx;
	char	*p;
	int    n, i, j, k;
	struct procentry64 *pe;
	
	pe = &P; 

	prog = basename(argv[0]);

	opterr = 0;

	optstr = PARGS_OPTS;

	while ((c = getopt(argc, argv, optstr)) != -1)
		switch (c) {
		case 'a':
			aflg++;
			break;
		case 'c':
			cflg++;
			break;
		case 'e':
			eflg++;
			break;
		case 'F':
			Fflg++;
			break;
		case 'l':
			lflg++;
			break;
		case 'x':
			xflg++;
			break;
		case 'r':
			rflg++;
			break;
		case '?':
			errflg++;
			break;
		}

	if ((argc - optind) != 1) 
		errflg++;

	if (!(aflg||cflg||eflg||Fflg||xflg||rflg))
		aflg++;

	if (errflg) {
		fprintf(stderr, "%s: %s\n", prog, PARGS_USAGE);
		exit(2);
	}

	if ((idx = atoi(argv[optind])) == 0) {
		fprintf(stderr, "%s: invalid number specified for pid \n", prog);
		exit(1);
	}

	if ((n = getprocs64(pe, sizeof(struct procentry64),
						(struct fdsinfo64 *)NULL, 0, &idx, 1)) != 1) {
		fprintf(stderr, "%s: failed to retrieve process %ld\n", prog, idx);
		exit(1);
	}

	if (!aflg)
		printf("Command [%s] %s\n", pe->pi_comm, p);

	if (aflg) {
		if (getargs(pe, sizeof(struct procentry64),
					(char *)args_buf, ARG_MAX) != 0) {
			perror("getargs");
			exit(1);
		}
		printarglist((char *)args_buf, lflg ? ' ' : '\n');
	}

	if (rflg) {
		printf("%20s %-20s %-20s\n", "",  "soft" , "hard");
		printrlimits(pe, RLIMIT_CPU);
		printrlimits(pe, RLIMIT_FSIZE);
		printrlimits(pe, RLIMIT_DATA);
		printrlimits(pe, RLIMIT_STACK);
		printrlimits(pe, RLIMIT_RSS);
		printrlimits(pe, RLIMIT_CORE);
		printrlimits(pe, RLIMIT_NOFILE);
		printrlimits(pe, RLIMIT_THREADS);
		printrlimits(pe, RLIMIT_NPROC);
	}

	if (eflg) {
		if ((n = getevars(pe, sizeof(struct procentry64),
							(char *)buf, ARG_MAX)) != 0) {
			perror("getevars");
			exit(n);
		}
		printarglist((char *)buf, '\n');
	}

	exit(0);
}

static void
printrlimits(struct procentry64 *pe, int r)
{
	int div = 1;

	switch (r) {
	case RLIMIT_CPU:	/* cpu time in milliseconds */
		printf("%-20s ", "time(seconds)");
		div = 1000;
		break;
	case RLIMIT_FSIZE:	/* maximum file size - blocks */
		printf("%-20s ", "files(blocks)");
		div = 512;
		break;
	case RLIMIT_CORE:	/* core file size - blocks */
		printf("%-20s ", "coredump(blocks)");
		div = 512;
		break;
	case RLIMIT_DATA:	/* data size - KB */
	case RLIMIT_STACK:  /* stack size - KB */
	case RLIMIT_RSS:  	/* resident set size - KB */
	case RLIMIT_AS:   	/* max size proc's total memory--not enforced - KB*/
		switch(r) {
		case RLIMIT_DATA:
			printf("%-20s ", "data(kbytes)");
			break;
		case RLIMIT_STACK:
			printf("%-20s ", "stack(kbytes)");
			break;
		case RLIMIT_RSS:  
			printf("%-20s ", "memory(kbytes)");
			break;
		case RLIMIT_AS:   	
			printf("%-20s ", "as(kbytes)");
			break;
		}
		div = 1024;
		break;
	case RLIMIT_NOFILE:  /* max # allocated fds--not enforced */
		printf("%-20s ", "nofiles(descriptors)");
		break;
	case RLIMIT_THREADS:  /* max # threads per process */
		printf("%-20s ", "threads(per process)");
		break;
	case RLIMIT_NPROC:    /* max # processes per user  */
		printf("%-20s ", "processes(per user)");
		break;
	default:
		break;
	}
	if UNLIMITED(pe->pi_rlimit[r].rlim_cur)
		printf("%-20s ", "unlimited");
	else
		printf("%-20lld ", pe->pi_rlimit[r].rlim_cur/div);
	if UNLIMITED(pe->pi_rlimit[r].rlim_max) 
		printf("%-20s ", "unlimited");
	else
		printf("%-20lld ", pe->pi_rlimit[r].rlim_max/div);
	printf("\n");
}

static void
printarglist(const char *s, const char sep)
{
	const char *p = s;

	while (*p != '\0') {
		printf("%s%c", p, sep);
		while (*(p++) != '\0')
			;
	}
	printf("%s", (sep == '\n') ? "" : "\n");
}
