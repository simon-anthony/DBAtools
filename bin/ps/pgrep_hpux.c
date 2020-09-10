#ifndef _HPUX_SOURCE
#pragma ident "$Header$"
#else
static char *svnid = "$Header$";
#endif

/* pgrep
 *   Minimal implementation of pgrep(1) for HP-UX. Not all options supported,
 *   but the most useful ones are there.
 *   Simon Anthony
 */

#define PGREP_USAGE "[-flvx] [-n|-o] [-d dlim] [-g grouplist] [-G rgrouplist] [-P ppidlist] [-s sidlist] [-S statelist] [-t termlist] [-u userlist] [-U ruserlist] [pattern]"
#define PKILL_USAGE "[-signal] [-fvx] [-n|-o] [-g grouplist] [-G rgrouplist] [-P ppidlist] [-s sidlist] [-S statelist] [-t termlist] [-u userlist] [-U ruserlist] [pattern]"

#define USAGE_HPUX "[-flvx] [-n|-o] [-d dlim] [-e etimelist] [-g pgrplist] [-G grouplist] [-P ppidlist] [-R prmgrplist] [-s sidlist] [-S statelist] [-t termlist] [-u userlist] [-U ruserlist] [-z vszlist] [-Z psetlist] [pattern]"
#define USAGE_PKILL_HPUX "[-fvx] [-n|-o] [-e etimelist] [-g pgrplist] [-G grouplist] [-P ppidlist] [-R prmgrplist] [-s sidlist] [-S statelist] [-t termlist] [-u userlist] [-U ruserlist] [-z vszlist] [-Z psetlist] [pattern]"

#define PGREP_OPTS "flvxnod:O:P:g:G:s:u:U:S:t:"
#define PKILL_OPTS "fvxnoO:P:g:G:s:u:U:S:t:"

#include <sys/param.h>
#include <sys/pstat.h>
#include <sys/dk.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <regex.h>
#include <pwd.h>
#include <grp.h>

#define min(a, b)      ((a) > (b) ? (b) : (a))
#define max(a, b)      ((a) < (b) ? (b) : (a))

#ifdef __STDC__
static char *ttynam(struct __psdev);
extern int str2sig(const char *, int *);
#else
static char *ttynam();
extern int str2sig();
#endif

static char    *prog;
//#define MAX_LENGTH 1024
static char    long_command[ARG_MAX];
static union pstun pu;

static char        	   *ere = ".*";
static regex_t         re;                                                   /* compile struct */

/*static regmatch_t    pm[_REG_SUBEXP_MAX + 1];       array of match structs */
static regmatch_t      pm;                                                   /* match struct */

static int ppidlist[BUFSIZ];
static int nppids = 0;

static int sidlist[BUFSIZ];
static int nsids = 0;

static int egidlist[BUFSIZ];
static int negids = 0;

static int gidlist[BUFSIZ];
static int ngids = 0;

static int euidlist[BUFSIZ];
static int neuids = 0;

static int uidlist[BUFSIZ];
static int nuids = 0;

static int statelist[BUFSIZ];
static int nstates = 0;

static char *ttylist[BUFSIZ];
static int nttys = 0;

static int fflg = 0, lflg = 0, vflg = 0, xflg = 0, nflg = 0, oflg = 0, dflg = 0,
				  Pflg = 0, sflg = 0, gflg = 0, Gflg = 0, uflg = 0, Uflg = 0, tflg = 0,
				  Sflg = 0, errflg = 0;

static char dlim = '\n';

static int sig_to_send = SIGTERM;	/* Signal to send */

/* used for -n/-o */
static _T_LONG_T       saved_start = 0;
static _T_LONG_T    saved_pid;
static char         saved_arg[ARG_MAX];

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int    argc;
 char   *argv[];
#endif
{
	char    *p;
	extern  char *optarg;
	extern  int optind, opterr;
	char	*optstr;
	char    *options, *value;
	int		c;
	char    buf[BUFSIZ];
	char    *arg, *args;
	pid_t   pid;
	struct pst_static pss;
	struct pst_dynamic psd;
	struct pst_status *pst = NULL;
	struct pst_status *pstp = NULL;
	struct passwd *pwd;
	struct group *grp;
	int    n, i, j, k;
	int    found = 0;
	int    matches = 0;

	prog = basename(argv[0]);

	opterr = 0;

	if (strcmp(prog, "pkill") == 0) {
		if (argc > 1 && argv[1][0] == '-' &&
			str2sig(&argv[1][1], &sig_to_send) == 0) {
				argv[1] = argv[0];
				argv++;
				argc--;
		}
		optstr = PKILL_OPTS;
	} else
		optstr = PGREP_OPTS;

	while ((c = getopt(argc, argv, optstr)) != -1)
		switch (c) {
		case 'n':
			nflg++;
			if (oflg) errflg++;
			break;
		case 'o':
			oflg++;
			if (nflg) errflg++;
			break;
		case 'f':
			fflg++;
			break;
		case 'l':
			lflg++;
			break;
		case 'v':
			vflg++;
			break;
		case 'x':
			xflg++;
			break;
		case 'd':
			dflg++;
			dlim = optarg[0];
			break;
		case 'P':              /* csv list of ppids */
			Pflg++;
			args = optarg; /* parse ppids */

			while ((arg = strtok(args, ", ")) != NULL && nppids < BUFSIZ) {
					if (n = atoi(arg)) 
							ppidlist[nppids++] = n;
					args = NULL;
			}
			break;
		case 's':              /* csv list of session ids */
			sflg++;
			args = optarg; /* parse session ids */

			while ((arg = strtok(args, ", ")) != NULL && nsids < BUFSIZ) {
					if (n = atoi(arg)) 
							sidlist[nsids++] = n;
					else {
						fprintf(stderr, "%s: invalid argument %s for -s option\n", prog,  arg);
						exit(2);
					}
					args = NULL;
			}
				break;
		case 'g':              /* csv list of effective group ids TODO: NO!
		this should be process group list */
		case 'G':              /* csv list of real group ids */
			(c == 'g') ? gflg++ : Gflg++;
			args = optarg; /* parse session egids */

			while ((arg = strtok(args, ", ")) != NULL && ((c == 'u') ? negids < BUFSIZ : ngids < BUFSIZ)) {
				if (n = atoi(arg)) {
					(c == 'g') ? (egidlist[negids++] = n)
								: (gidlist[ngids++] = n);
				} else {
					if ((grp = getgrnam(arg)) != NULL) {
						(c == 'g') ? (egidlist[negids++] = grp->gr_gid)
									: (gidlist[ngids++] = grp->gr_gid);
					} else {
						fprintf(stderr, "%s: unknown group %s\n", prog, arg);
						exit(1);
					}
				}
				args = NULL;
			}
			break;
		case 'u':              /* csv list of effective user ids */
		case 'U':              /* csv list of real user ids */
			(c == 'u') ? uflg++ : Uflg++;
			args = optarg; /* parse session euids */

			while ((arg = strtok(args, ", ")) != NULL && ((c == 'u')  ? neuids < BUFSIZ : nuids < BUFSIZ)) {
				if (n = atoi(arg)) {
					(c == 'u') ? (euidlist[neuids++] = n)
								: (uidlist[nuids++] = n);
				} else {
					if ((pwd = getpwnam(arg)) != NULL) {
						(c == 'u') ? (euidlist[neuids++] = pwd->pw_uid)
									: (uidlist[nuids++] = pwd->pw_uid);
					} else {
						fprintf(stderr, "%s: unknown user %s\n", prog, arg);
						exit(1);
					}
				}
				args = NULL;
			}
			break;
		case 'S':              /* csv list of states */
			Sflg++;
			args = optarg; /* parse states */
			while ((arg = strtok(args, ", ")) != NULL && nstates < BUFSIZ) {
				switch(args[0]) {
				case 'R':
					statelist[nstates++] = PS_RUN;
					break;
				case 'S':
					statelist[nstates++] = PS_SLEEP;
					break;
				case 'T':
					statelist[nstates++] = PS_STOP;
					break;
				case 'Z':
					statelist[nstates++] = PS_ZOMBIE;
					break;
				default:
					fprintf(stderr, "%s: unknown state %s\n", prog, arg);
					exit(2);
				}
				args = NULL;
			}
			break;
		case 't':              /* csv list of ttys */
			tflg++;
			args = optarg; /* parse ttys */
			while ((arg = strtok(args, ", ")) != NULL && nttys < BUFSIZ) {        /* don't bother to check if tty exists */
				ttylist[nttys++] = strdup(arg);
				args = NULL;
			}
			break;
		case '?':
			errflg++;
			break;
		}

	if (!(Pflg||Sflg||sflg||Gflg||gflg||Uflg||uflg||tflg) && !(argc - optind) == 1) {
		if (fflg||lflg||dflg||xflg||vflg)
			fprintf(stderr, "%s: no matching criteria specified\n", prog);
		errflg++;
	}

	if (argc - optind > 1)
		errflg++;

	if (errflg) {
		if (strcmp(prog, "pkill") == 0) 
			fprintf(stderr, "%s: %s\n", prog, PKILL_USAGE);
		else
			fprintf(stderr, "%s: %s\n", prog, PGREP_USAGE);
		exit(2);
	}

	if (argc - optind == 1) {
		if (xflg) {
			if ((ere = (char *) malloc(strlen(argv[optind]) + 3)) == NULL) {
					perror("malloc");
					exit(3);
			}
			sprintf(ere, "^%s$", argv[optind]);   /* exact match */
		} else
			ere = argv[optind];
	}

	if ((pid = getpid()) < 0) {
		perror("getpid");
		exit(3);
	}

	if ((n = regcomp(&re, ere, REG_EXTENDED|REG_NOSUB) != 0)) {
		regerror(n, &re, buf, sizeof buf);
		printf("%s\n", buf);
		exit(3);
	}

	if (pstat_getstatic(&pss, sizeof(pss), (size_t)1, 0) == -1) {
		perror("pstat_getstatic");
		exit(3);
	}

	if (fflg)
		pu.pst_command = long_command;

	if (pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1) {
		perror("pstat_getdynamic");
		exit(3);
	}

	if ((pst = (struct pst_status *)malloc(sizeof(struct pst_status) * psd.psd_maxprocs)) == NULL) {
		perror("malloc");
		exit(3);
	}

	if ((n = pstat_getproc(pst, sizeof(struct pst_status), psd.psd_activeprocs, 0)) == -1) {
		perror("pstat_getproc");
		exit(3);
	}

	pstp = pst;

	for (j = 0; j < n; j++, pstp++) {
		if (pid == pstp->pst_pid)
			continue;      // not me!
		if (Pflg) {
			found = 0;
			for (i = 0; i < nppids && i < BUFSIZ; i++)
				if (pstp->pst_ppid == ppidlist[i]) {
					found++;
					break;
				}
			if (!found)
					continue;
		}
		if (sflg) {
			found = 0;
			for (i = 0; i < nsids && i < BUFSIZ; i++)
				if (pstp->pst_sid == sidlist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (gflg) {            /* effective group list */
			found = 0;
			for (i = 0; i < negids && i < BUFSIZ; i++)
				if (pstp->pst_egid == egidlist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (Gflg) {            /* real group list */
			found = 0;
			for (i = 0; i < ngids && i < BUFSIZ; i++)
				if (pstp->pst_gid == gidlist[i]) {
					found++;
					break;
				}
			if (!found)
					continue;
		}
		if (uflg) {            /* effective user list */
			found = 0;
			for (i = 0; i < neuids && i < BUFSIZ; i++)
				if (pstp->pst_euid == euidlist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (Uflg) {            /* real user list */
			found = 0;
			for (i = 0; i < nuids && i < BUFSIZ; i++)
				if (pstp->pst_uid == uidlist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (Sflg) {            /* state list */
			found = 0;
			for (i = 0; i < nstates && i < BUFSIZ; i++)
				if (pstp->pst_stat == statelist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (tflg) {            /* tty list */
			/* Compare:
			 *   o) the device's file name (such as tty04), or
			 *
			 *   o) if the device's file name starts with tty, just the rest of
			 *      it (such as 04).
			 *
			 *   If the device's file is in a directory other than /dev or
			 *   /dev/pty, the terminal identifier must include the name of the
			 *   directory under /dev that contains the
			 *   device file (such as pts/5).
			 */
			char tty[TTY_NAME_MAX+1];
			found = 0;
			for (i = 0; i < nttys && i < BUFSIZ; i++) {
				p = ttynam(pstp->pst_term);
				if (strcmp(p, strncat(strncpy(tty, "/dev/",        TTY_NAME_MAX), ttylist[i], TTY_NAME_MAX)) == 0 ||
					strcmp(p, strncat(strncpy(tty, "/dev/pty/tty", TTY_NAME_MAX), ttylist[i], TTY_NAME_MAX)) == 0 ||
					strcmp(p, strncat(strncpy(tty, "/dev/tty",     TTY_NAME_MAX), ttylist[i], TTY_NAME_MAX)) == 0 ||
					strcmp(p, strncat(strncpy(tty, "/dev/pty",     TTY_NAME_MAX), ttylist[i], TTY_NAME_MAX)) == 0 ||
					strcmp(p, strncat(strncpy(tty, "/dev/pts/",    TTY_NAME_MAX), ttylist[i], TTY_NAME_MAX)) == 0 ||
					strcmp(p, ttylist[i]) == 0)
					found++;
					break;
			}
			if (!found)
				continue;
		}
		if (fflg) {    /* match against entire args */
			if (pstat(PSTAT_GETCOMMANDLINE, pu, ARG_MAX, 1, pstp->pst_pid) == -1) {
#ifdef FAIL
				perror("pstat_getcommandline");
				exit(3);
#else
				continue;              /* process has terminated */
#endif
			}
			p = pu.pst_command;
		} else
			p = pstp->pst_ucomm;   /* not the longer form: pstp->pst_cmd */
		found = 0;
		if (regexec(&re, p, 1, &pm, 0) == 0 && ! vflg)
			found++;
		else if (regexec(&re, p, 1, &pm, 0) != 0 && vflg) 
			found++;
		if (found) {
			if (nflg||oflg) {
				if (!saved_start) {
					saved_start = pstp->pst_start;
					strncpy(saved_arg, p, ARG_MAX);
				} else {
					if (nflg) {
						if (pstp->pst_start > saved_start) {  /* newer */
							saved_pid = pstp->pst_pid;
							saved_start = pstp->pst_start;
							strncpy(saved_arg, p, ARG_MAX);
						}
					} else if (oflg) {
						if (pstp->pst_start < saved_start) {  /* older */
							saved_start = pstp->pst_start;
							saved_pid = pstp->pst_pid;
							strncpy(saved_arg, p, ARG_MAX);
						} 
					} 
				} 
			} 
			if (!(nflg||oflg)) {
				if (strcmp(prog, "pkill") == 0) {
					if (kill(pstp->pst_pid, sig_to_send) != 0) {
						fprintf(stderr, "failed to kill %*lld\n" , 5,
						pstp->pst_pid);
					}
				} else {
					printf("%*lld", 5, pstp->pst_pid);
					if (lflg)
						printf(" %s", p);
					printf("%c", dlim);
				}
			}
			matches++;
		}
	}

	if (nflg||oflg) {
		if (saved_start) {
			if (strcmp(prog, "pkill") == 0)
				if (kill(saved_pid, sig_to_send) != 0) {
					fprintf(stderr, "failed to kill %*lld\n" , 5, saved_pid);
			} else {
				printf("%*ld", 5, (int)saved_pid); // changed
				if (lflg)
					printf(" %s\n", saved_arg);
			}
		}
	}

	matches > 0 ? exit(0) : exit(1);
}
#include <devnm.h>
static char *
#ifdef __STDC__
ttynam(struct __psdev p)
#else
ttynam(p, width)
struct __psdev p;
#endif
{
	static char ttydev[MAXPATHLEN+1];
	char *s;

	if (p.psd_major < 0 || p.psd_minor < 0 ) 
		goto notty;

	if (devnm(S_IFCHR, (dev_t)((p.psd_major<<24)|p.psd_minor), ttydev, sizeof(ttydev), 1) == 0) {
		s = ttydev;
		return ttydev;
	}
notty:
	return NULL;
}
