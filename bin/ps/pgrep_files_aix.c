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

#define PGREP_USAGE "[-flvx] [-n|-o] [-d dlim] [-g grouplist] [-G rgrouplist] [-P ppidlist] [-s sidlist] [-S statelist] [-t termlist] [-u userlist] [-U ruserlist] [pattern]"
#define PKILL_USAGE "[-signal] [-fvx] [-n|-o] [-g grouplist] [-G rgrouplist] [-P ppidlist] [-s sidlist] [-S statelist] [-t termlist] [-u userlist] [-U ruserlist] [pattern]"

#define PGREP_OPTS "flvxnod:O:P:g:G:s:u:U:S:t:"
#define PKILL_OPTS "fvxnoO:P:g:G:s:u:U:S:t:"

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
#include <regex.h>
#include <pwd.h>
#include <grp.h>

#define min(a, b)      ((a) > (b) ? (b) : (a))
#define max(a, b)      ((a) < (b) ? (b) : (a))

#define COUNT 12

static char    *prog;
#define MAX_LENGTH 1024

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
static u_longlong_t		saved_start = 0;
static u_longlong_t		saved_pid;
static char         saved_arg[ARG_MAX];
									/* for saved process in comparison -n|-o */

static char         args_buf[ARG_MAX];
									/* for getargs() output */

static int nP = 0;					/* number of processes */
static struct procentry64 *P = (struct procentry64 *)NULL;
									/* process table */
static struct fdsinfo64 *F = (struct fdsinfo64 *)NULL;
									/* file table */

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int    argc;
 char   *argv[];
#endif
{
	char    *rp;
	extern  char *optarg;
	extern  int optind, opterr;
	char	*optstr;
	char    *options, *value;
	int		c;
	char    buf[BUFSIZ];
	char    *arg, *args;
	pid_t   pid, idx;
	struct procentry64 *p;
	struct fdsinfo64 *f;
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
		case 'g':              /* csv list of effective group ids */
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
				case 'O':
					statelist[nstates++] = SNONE /* Nonexistent */;
					break;
				case 'A':
					statelist[nstates++] = SACTIVE /* Active */;
					break;
				case 'I':
					statelist[nstates++] = SIDL /* Idle */;
					break;
				case 'T':
					statelist[nstates++] = SSTOP /* Stopped */;
					break;
				case 'Z':
					statelist[nstates++] = SZOMB /* Canceled */;
					break;
				case 'W':
					statelist[nstates++] = SSWAP /* Swapped */;
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
			fprintf(stderr, "%s: -t not yet implemented\n", prog);
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



	if ((P = (struct procentry64 *)malloc((sizeof(struct procentry64)) * COUNT)) == NULL) {
		perror("malloc");
		exit(1);
	}
	if ((F = (struct fdsinfo64 *)malloc((sizeof(struct fdsinfo64)) * COUNT)) == NULL) {
		perror("malloc");
		exit(1);
	}

	n = idx = 0;
	nP = 0;
	p = P;
	f = F;

	/*while ((i = getprocs64(p, sizeof(struct procentry64), (struct fdsinfo64 *)NULL, 0, &idx, COUNT) == COUNT)) { */
	while ((i = getprocs64(p, sizeof(struct procentry64), f, sizeof(struct fdsinfo64), &idx, COUNT) == COUNT)) { 
		nP += COUNT;
		n++;
		if ((P = (struct procentry64 *)realloc(P, sizeof(struct procentry64) * (nP + COUNT))) == NULL) {
			perror("realloc");
			exit(1);
		}
		p = (struct procentry64 *)((char *)P + (n * COUNT * sizeof(struct procentry64)));
		if ((F = (struct fdsinfo64 *)realloc(F, sizeof(struct fdsinfo64) * (nP + COUNT))) == NULL) {
			perror("realloc");
			exit(1);
		}
		f = (struct fdsinfo64 *)((char *)F + (n * COUNT * sizeof(struct fdsinfo64)));

	}
	if (i > 0)
		nP += i;

	for (p = P, n = 0, f = F; n < nP; n++, p++, f++) { 

		if ((pid32_t)pid == p->pi_pid)
			continue;		/* not me! */
		if (Pflg) {
			found = 0;
			for (i = 0; i < nppids && i < BUFSIZ; i++)
				if (p->pi_ppid == ppidlist[i]) {
					found++;
					break;
				}
			if (!found)
					continue;
		}
		if (sflg) {
			found = 0;
			for (i = 0; i < nsids && i < BUFSIZ; i++)
				if (p->pi_sid == sidlist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (gflg) {            /* effective group list */
			found = 0;
			for (i = 0; i < negids && i < BUFSIZ; i++)
				if (p->pi_cred.crx_gid  == egidlist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (Gflg) {            /* real group list */
			found = 0;
			for (i = 0; i < ngids && i < BUFSIZ; i++)
				if (p->pi_cred.crx_rgid == gidlist[i]) {
					found++;
					break;
				}
			if (!found)
					continue;
		}
		if (uflg) {            /* effective user list */
			found = 0;
			for (i = 0; i < neuids && i < BUFSIZ; i++)
				if (p->pi_cred.crx_uid == euidlist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (Uflg) {            /* real user list */
			found = 0;
			for (i = 0; i < nuids && i < BUFSIZ; i++)
				/*if (p->pi_uid == uidlist[i]) { */
				if (p->pi_cred.crx_ruid == uidlist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (Sflg) {            /* state list */
			found = 0;
			for (i = 0; i < nstates && i < BUFSIZ; i++)
				if (p->pi_state == statelist[i]) {
					found++;
					break;
				}
			if (!found)
				continue;
		}
		if (tflg) {            /* tty list */
			found = 0;
			if (!found)
				continue;
		}

		if (fflg||lflg) {    /* match against entire args */
			if (getargs(p, sizeof(struct procentry64), (char *)args_buf, ARG_MAX) != 0) {
				perror("getargs");
				continue;
			}
			rp = (char *)args_buf;
			while (rp[0] != '\0') {
				while (rp[0] != '\0')
					rp++;
				rp[0] = ' ';
				rp++;
			}
			rp = (char *)args_buf;
		} 
		if (!fflg)
			rp = p->pi_comm;

		found = 0;
		if (regexec(&re, rp, 1, &pm, 0) == 0 && ! vflg)
			found++;
		else if (regexec(&re, rp, 1, &pm, 0) != 0 && vflg) 
			found++;
		if (found) {
		/*	printf("%d %s\n", p->pi_pid, (char *)p->pi_comm);*/
			if (nflg||oflg) {
				if (!saved_start) {
					saved_start = p->pi_start;
					strncpy(saved_arg, rp, ARG_MAX);
				} else {
					if (nflg) {
						if (p->pi_start > saved_start) {  /* newer */
							saved_pid = p->pi_pid;
							saved_start = p->pi_start;
							strncpy(saved_arg, rp, ARG_MAX);
						}
					} else if (oflg) {
						if (p->pi_start < saved_start) {  /* older */
							saved_start = p->pi_start;
							saved_pid = p->pi_pid;
							strncpy(saved_arg, rp, ARG_MAX);
						} 
					} 
				} 
			} 
			if (!(nflg||oflg)) {
				if (strcmp(prog, "pkill") == 0) {
					if (kill(p->pi_pid, sig_to_send) != 0) {
						fprintf(stderr, "failed to kill %*lld\n" , 5,
						p->pi_pid);
					}
				} else {
					if (matches > 0 && dflg)
						putchar(dlim);

					printf("%*ld", 5, p->pi_pid); 
					/*printf("[%s]", ttyname(p->pi_ttyd));    */
					printf("[%s]", ttyname(f->pi_ufd[0].fp));    
					if (lflg)
						printf(" %s", (char *)args_buf);
					if (!dflg)
						putchar(dlim);
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
				printf("%*ld", 5, (int)saved_pid); /* changed */
				if (lflg)
					printf(" %s\n", saved_arg);
			}
		}
	}

	matches > 0 ? exit(0) : exit(1);
}
