#ifndef _HPUX_SOURCE
#pragma ident "$Header$"
#else
static char *svnid = "$Header$";
#endif

/*
 * ptree: for 11.11
 */

#define USAGE "[-x] pid..."

#include <sys/param.h>
#include <sys/pstat.h>
#include <sys/dk.h>

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

#define	min(a, b)	((a) > (b) ? (b) : (a))
#define	max(a, b)	((a) < (b) ? (b) : (a))

static char *prog;
static long sc_page_size;
static int page_size;
static char long_command[ARG_MAX];
static union pstun pu;

int extended = 0;	/* available externally */

static int pids[BUFSIZ];
static int npids = 0;
static int nppids;
static int width = 10;

static struct pst_static pss;
static struct pst_dynamic psd;
static struct pst_status *pst = NULL;

static int oflg = 0, pflg = 0, xflg = 0, errflg = 0;

#ifdef __STDC__
static void printproc(struct pst_status *, int);
static void findchildren(struct pst_status *, int);
static int isnumber(char *);
#else
static void printproc();
static void findchildren();
static int isnumber();
#endif

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int	argc;
 char 	*argv[];
#endif
{
	char	*p;
	extern	char *optarg;
	extern	int optind, opterr;
	char	*options, *value;
	int		c;
	char	buf[BUFSIZ];
	char	*arg, *args;
	int		pid, ppid;
	struct pst_status *pstp = NULL;
	int		i, j, k;
	int		found = 0;
	int		depth = 0;
	int *ppids;


	prog = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "x")) != -1)
		switch (c) {
		case 'x':
			xflg++;
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

	if ((sc_page_size = sysconf(_SC_PAGE_SIZE)) == -1) {
		perror("sysconf");
		exit(1);
	}

	if (pstat_getstatic(&pss, sizeof(pss), (size_t)1, 0) == -1) {
		perror("pstat_getstatic");
		exit(1);
	}
	page_size = pss.page_size;

	if (xflg)
		pu.pst_command = long_command;

	if (pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1) {
		perror("pstat_getdynamic");
		exit(1);
	}

	if ((pst = (struct pst_status *)malloc(sizeof(struct pst_status) *
		psd.psd_maxprocs)) == NULL) {
		perror("malloc");
		exit(1);
	}

	if ((npids = pstat_getproc(pst, sizeof(struct pst_status),
		psd.psd_activeprocs, 0)) == -1) { 
		perror("pstat_getproc");
		exit(1);
	}

	if ((ppids = (int *)malloc(sizeof(int) * psd.psd_maxprocs)) == NULL) {
		perror("malloc");
		exit(1);
	}

	for (i = optind; i < argc; i++) {
		pstp = pst;

		if (!isnumber(argv[i]) || (pid = atoi(argv[i])) == 0 || pid == LONG_MAX || pid == LONG_MIN ) {
			fprintf(stderr, "%s: Invalid username %s\n", prog, argv[i]);
			continue;
		}

		found = 0;

		for (j = 0; j < npids; j++, pstp++) {
			if (pstp->pst_pid != pid)
				continue;

			pid = pstp->pst_pid;
			found++;
		}

		if (found) {
			ppid = pid;
			pstp = pst;
			nppids = 0;

			/* find my parent; and my parent's parent; and my parent's parent's
			 * parent ...
			 */
			for (j = 0; j < npids; j++, pstp++) {
				if (pstp->pst_pid != ppid)
					continue;
				if (pstp->pst_pid == 1)		/* found him - start again */
					break;
				depth++;
				ppids[nppids++] = pstp->pst_pid;
				j = 0;
				ppid = pstp->pst_ppid;
				pstp = pst;
			}
			if (nppids-1 == 0) {
				/* print all processes who have me as parent */
				pstp = pst;
				for (k = 0; k < npids; k++, pstp++)
					if (pstp->pst_ppid == 1 && pstp->pst_pid == pid)
						findchildren(pstp, nppids-1);
			} else {

			/* print parents, grandparents... */
			for (j = nppids-1; j >= 0; j--) {
				pstp = pst;
				for (k = 0; k < npids; k++, pstp++)
					if (pstp->pst_pid == ppids[j] && pstp->pst_pid != pid)
						printproc(pstp, nppids-j-1);

				/* print all processes who have (new) me as parent */
				pstp = pst;
				for (k = 0; k < npids; k++, pstp++)
					if (pstp->pst_ppid == ppids[j] && pstp->pst_pid == pid)
						findchildren(pstp, nppids-j);
			}
			}
		} else {
			(void) printf("%s: pid %*d non existent\n",
				prog, width, atoi(argv[i]));
		}
	}

	exit(0);
}

static void
#ifdef __STDC__
printproc(struct pst_status *pstp, int indent)
#else
printproc(pstp, indent)
 struct pst_status *pstp;
 int indent;
#endif
{
	(void) printf("%*s%-*lld ", indent, "", width, pstp->pst_pid);

	if (xflg)
		if (pstat(PSTAT_GETCOMMANDLINE, pu, ARG_MAX, 1, pstp->pst_pid) == -1) {
			perror("pstat_getcommandline");
			exit(1);
		}
	(void) printf("%s\n", xflg ? pu.pst_command : pstp->pst_cmd);
}

static void
#ifdef __STDC__
findchildren(struct pst_status *pstp, int indent)
#else
findchildren(pstp, indent)
 struct pst_status *pstp;
 int indent;
#endif
{
	int i;
	struct pst_status *p = pst;

	printproc(pstp, indent);

	for (i = 0; i < npids; i++, p++)
		if (p->pst_ppid == pstp->pst_pid)
			findchildren(p, indent + 1);
}
static int
#ifdef __STDC__
isnumber(char *s)
#else
isnumber(s)
 char *s;
#endif 
{
	char *p = s;

	while (*s)
		if (!isdigit(*(s++)))
			return 0;
	return 1;
}
