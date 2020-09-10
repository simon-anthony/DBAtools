#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

/*
 * ptree: for AIX
 */

#define USAGE "[-h|-s] [-x] pid..."

#include <sys/param.h>
#include <procinfo.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <pwd.h>
#include <ctype.h>
#include <limits.h>

#define	min(a, b)	((a) > (b) ? (b) : (a))
#define	max(a, b)	((a) < (b) ? (b) : (a))
#define COUNT 12

static char *prog;
static char args_buf[ARG_MAX];
									/* for getargs() output */

int extended = 0;	/* available externally */

static int npids = 0;					/* number of processes */
static int nppids;
static int width = 10;

static struct procentry64 *pst = (struct procentry64 *)NULL;
static int *ppids;

static int sflg = 0, aflg = 0, xflg = 1, hflg = 0, errflg = 0;

static void printproc(struct procentry64 *, int);
static void findchildren(struct procentry64 *, int);
static int isnumber(char *);
static char *getcommand(struct procentry64 *);
static void printtree(pid32_t);

int
main(int argc, char *argv[])
{
	extern	char *optarg;
	extern	int optind, opterr;
	char	*options, *value;
	int		c;
	char	buf[BUFSIZ];
	char	*arg, *args;
	pid32_t	pid, ppid;
	pid_t	idx;
	struct procentry64 *pstp;
	int		n, i, j;

	prog = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "asxh")) != -1)
		switch (c) {
		case 'x':
			xflg++;
			break;
		case 's':
			if (hflg)
				errflg++;
			sflg++;
			break;
		case 'a':
			if (hflg)
				errflg++;
			aflg++;
			break;
		case 'h':	/* hide root processes */
			if (aflg||sflg)
				errflg++;
			hflg++;
			break;
		case '?':
			errflg++;
			break;
		}

	if (argc-optind < 1)
		errflg++;

	if (errflg) {
		fprintf(stderr, "%s: %s\n", prog, USAGE);
		exit(2);
	}

	/* Collect the process table... */
	n = idx = 0;
	npids = COUNT;
	pstp = pst;

	while ((i = getprocs64(pstp, sizeof(struct procentry64), (struct fdsinfo64 *)NULL, 0, &idx, COUNT) == COUNT)) {
		n += COUNT;
		if (n >= npids) {
			if ((pst = (struct procentry64 *)realloc((char *)pst, sizeof(struct procentry64) * (npids + COUNT))) == NULL) {
				perror("realloc");
				exit(1);
			}
			npids += COUNT;
		}
		pstp = (struct procentry64 *)((char *)pst + (n * sizeof(struct procentry64)));

	}
	if (i > 0)
		n += i;

	if ((ppids = (pid32_t *)malloc(sizeof(pid32_t) * CHILD_MAX)) == NULL) {
		perror("malloc");
		exit(1);
	}

	for (i = optind; i < argc; i++) {

		struct passwd *pwd = NULL;

		if (!isnumber(argv[i]) || (pid = atoi(argv[i])) == 0 || pid == LONG_MAX || pid == LONG_MIN ) {

			if ((pwd = getpwnam(argv[i])) == NULL) { /*  pwd->pw_uid */
				fprintf(stderr, "%s: Invalid username %s\n", prog, argv[i]);
				continue;
			}
		}


		for (j = 0, pstp = pst; j < npids; j++, pstp++) {

			if (pwd != NULL) {
				if (pstp->pi_uid != pwd->pw_uid)
					continue;
				/* if my parent's parent's pid is init (or my parent's pid for detached process)
					- I am the last in the chain */
				if (pstp->pi_pid == pstp->pi_sid || pstp->pi_ppid == (aflg ? 0 : 1))
					printtree(pstp->pi_pid);
			} else {
				if (pstp->pi_pid != pid)
					continue;
				printtree(pstp->pi_pid);
			}
		}
	}

	exit(0);
}

static void
printproc(struct procentry64 *p, int indent)
{
	char *s = NULL;

	(void) printf("%*s%d ", indent, "", p->pi_pid);
	if (xflg) 
		s = getcommand(p);
	(void) printf("%s\n", xflg ? (strlen(s) ? s : (char *)p->pi_comm) : (char *)p->pi_comm);
}

static void
findchildren(struct procentry64 *pstp, int indent)
{
	int i;
	struct procentry64 *p = pst;

	printproc(pstp, indent);

	for (i = 0; i < npids; i++, p++)
		if (p->pi_ppid == pstp->pi_pid)
			findchildren(p, indent + 1);
}
static int
isnumber(char *s)
{
	char *p = s;

	while (*s)
		if (!isdigit(*(s++)))
			return 0;
	return 1;
}

static char *
getcommand(struct procentry64 *p)
{
	char *rp;

	if (getargs(p, sizeof(struct procentry64), (char *)args_buf, ARG_MAX) != 0) {
		perror("getargs");
		return NULL;
	}
	rp = (char *)args_buf;
	while (rp[0] != '\0') {
		while (rp[0] != '\0')
			rp++;
		rp[0] = ' ';
		rp++;
	}
	rp = (char *)args_buf;
	return rp;
}

static void
printtree(pid32_t pid) 
{
	pid32_t ppid = pid;
	struct procentry64 *pstp;
	int		depth = 0;
	int		i, j, k;
	nppids = 0;

	/* find my parent; and my parent's parent; and my parent's parent's
	 * parent ...
	 */
	for (j = 0, pstp = pst; j < npids; j++, pstp++) {

		if (pstp->pi_pid != ppid)
			continue;
		if (pstp->pi_pid == 1 || (hflg && pstp->pi_uid == 0))		/* found him - start again */
		//if (pstp->pi_pid == (aflg ? 0 : 1) )
			break;
		depth++;
		ppids[nppids++] = pstp->pi_pid;
		j = 0;
		ppid = pstp->pi_ppid;
		pstp = pst;
	}
	if (nppids-1 == 0) {
		/* print all processes who have me as parent */
		for (k = 0, pstp = pst; k < npids; k++, pstp++)
			if (pstp->pi_ppid == 1 && pstp->pi_pid == pid)
				findchildren(pstp, nppids-1);
	} else {

		/* print parents, grandparents... */
		for (j = nppids-1; j >= 0; j--) {
			for (k = 0, pstp = pst; k < npids; k++, pstp++)
				if (pstp->pi_pid == ppids[j] && pstp->pi_pid != pid)
					printproc(pstp, nppids-j-1);

			/* print all processes who have (new) me as parent */
			for (k = 0, pstp = pst; k < npids; k++, pstp++)
				if (pstp->pi_ppid == ppids[j] && pstp->pi_pid == pid)
					findchildren(pstp, nppids-j);
		}
	}
}
