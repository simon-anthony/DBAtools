#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

/* pgrep
 *   Minimal implementation of pgrep(1) for AIX based on HP-UX arguments.
 *   Not all options supported, but the most useful ones are there.
 *   Simon Anthony
 */

/*
#define PARGS_USAGE "[-aceFlx] pid"
only -t supported at present
*/
#define PARGS_USAGE "-e pid"

#define PARGS_OPTS "aceFlx"

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

static int aflg = 0, eflg = 0, errflg = 0;

static struct procentry64 P; /* process */
static char buf[ARG_MAX];	 /* env vars list */
static char args_buf[ARG_MAX]; /* for getargs() output */


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

	prog = basename(argv[0]);

	opterr = 0;

	optstr = PARGS_OPTS;

	while ((c = getopt(argc, argv, optstr)) != -1)
		switch (c) {
		case 'e':
			eflg++;
			break;
		case 'a':
			aflg++;
			break;
		case '?':
			errflg++;
			break;
		}

	if (errflg) {
		fprintf(stderr, "%s: %s\n", prog, PARGS_USAGE);
		exit(2);
	}
	printf("%ld\n", getpid());
	/*
	printf("LDR_CNTRL=%s\n", getenv("LDR_CNTRL"));
	*/


	sleep(3);

	exit(0);
}
