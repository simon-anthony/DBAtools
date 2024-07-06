#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid_main = "$Header$";
#endif
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include "ctx.h"
#ifdef linux
#include <signal.h>
#endif


static void usage(char *s)
{
	fprintf(stderr,
		"usage: %s [-i] [-f] [[-v|-p]] <variable>] [<context_file>]\n", s);
	fprintf(stderr,
		"usage: %s [-i] [-f] -e <variable>=<value>... [<context_file>]\n", s);
	exit(2);
}

#define	STREAM		1
#define	WRITE		2

static Variable *variables[MAXVARS];

extern int yyparse();
#ifdef _HPUX_SOURCE
extern char *yylocale;
#endif

static int	edit = 0;
static char	*prog;
static char	*value = NULL;
char	*rval = NULL;

static int vflg = 0, pflg = 0, eflg = 0, iflg = 0, fflg = 0, errflg = 0;

#ifdef __STDC__
static int lock(char *);
static int addvar(int, char *, char *);
#else
static int lock();
static int addvar();
#endif

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	int		c, n = 0;
	char	*v;
	char	*path = NULL, *tmppath;
	extern	char *optarg;
	extern	int optind;
	FILE	*fp, *fpt;
	char	buf[BUFSIZ];

	prog = basename(argv[0]);

	#ifdef _HPUX_SOURCE
	setlocale (LC_ALL, yylocale);
	#endif
	variables[0] = NULL;

	ctxsetflags(CTX_PRINT);

	while ((c = getopt(argc, argv, "v:p:e:if?")) != -1 && n+1 < MAXVARS)
		switch (c) {
		case 'v':		/* print variable assignment */
			if ((n = addvar(n, optarg, NULL)) == 0)
				exit(1);
			vflg++;
			ctxsetflags(CTX_PRINTV);
			if (pflg || eflg)
				errflg++;
			break;
		case 'p':		/* variable is a regex */
			if ((n = addvar(n, optarg, NULL)) == 0)
				exit(1);
			ctxsetflags(CTX_REGEX);
			pflg++;
			if (vflg || eflg)
				errflg++;
			break;
		case 'e':		/* edit */
			if ((v = strtok(optarg, "=")) == NULL)
				usage(prog);
			if ((n = addvar(n, v, strtok(NULL, "="))) == 0)
				exit(1);
			eflg++;
			ctxsetflags(CTX_EDIT);
			if (vflg || pflg)
				errflg++;
			break;
		case 'i':		/* ignore CONTEXT_FILE */
			iflg++;
			break;
		case 'f':		/* insist on a file */
			fflg++;
			break;
		case '?':
			errflg++;
		}

	if (argc-optind > 2)
		errflg++;

	if (eflg && argc-optind == 1)
		edit = WRITE;
	else if (eflg && argc-optind == 0)
		edit = STREAM;
	else if (eflg)
		errflg++;

	if (vflg || pflg) {
		if (argc-optind > 1)
			errflg++;
		if (argc-optind == 1)
			path = argv[optind];
	} else if (eflg) {
		path = argv[optind];
	} else {
		if (argc-optind == 1) {
			if (access(argv[optind], R_OK) == 0)
				path = argv[optind];
			else {
				if ((n = addvar(n, argv[optind], NULL)) == 0)
					exit(1);
			}
		} else if (argc-optind == 2) {
			if ((n = addvar(n, argv[optind], NULL)) == 0)
				exit(1);
			path = argv[optind+1];
		}
	}
#ifdef _HPUX_SOURCE
	if (strcmp(path, "-v") == 0) {  /* getopt doesn't work */
		path = NULL;
		vflg++;
	}
#endif
	if (errflg)
		usage(prog);

	if (!path && !iflg)
		path = getenv("CONTEXT_FILE");

	if (path) {
		if (eflg && edit == WRITE) {
			if ((fp = fdopen(lock(path), "r")) == NULL) {
				fprintf(stderr, "%s: cannot open '%s' for locking\n",
					prog, path);
				exit(1);
			}
			if ((tmppath = tmpnam((char *)NULL)) == NULL) {
				fprintf(stderr, "%s: cannot create temporary file name\n",
					prog);
				exit(1);
			}
			if ((fpt = freopen(tmppath, "w", stdout)) == NULL) {
				fprintf(stderr, "%s: cannot open '%s'\n", prog, tmppath);
				exit(1);
			}
		}
		if ((fp = freopen(path, "r", stdin)) == NULL) {
			fprintf(stderr, "%s: cannot open '%s'\n", prog, path);
			exit(1);
		}
	} else if (fflg) {
		fprintf(stderr, "%s: context_file must be specified on comamnd line",
			prog);
		if (!iflg)
			fprintf(stderr, " or in environment");
		fprintf(stderr, "\n");
		exit(2);
	}

	ctxset(CTX_VAR, variables);
	//ctxset(CTX_NEWVAL, value);
	ctxsetedit(edit);

	if (!yyparse()) {
		if (variables[0] && !ctxfound())
			exit(1);
	} else
		exit(5);

	if (eflg && edit == WRITE) {
		fclose(fp);
		fclose(fpt);

		/* copy back */
		signal(SIGTERM, SIG_IGN);
		signal(SIGINT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);

		if ((fp = fopen(path, "w")) == NULL) {
			fprintf(stderr, "%s: cannot open '%s' for writing\n", prog, path);
			exit(1);
		}
		if ((fpt = fopen(tmppath, "r")) == NULL) {
			fprintf(stderr, "%s: cannot open '%s' for reading\n",
				prog, tmppath);
			exit(1);
		}
		while (fgets((char *)buf, BUFSIZ, fpt) != NULL)
			if (fputs((char *)buf, fp) == EOF) {
				perror("fputs");
				exit(1);
			}
		fclose(fp);
		fclose(fpt);

		if (unlink(tmppath) != 0) {
			 perror("unlink");
			 exit(1);
		}
	}
	exit(0);
}


#include <unistd.h>
#include <fcntl.h>
/*
 * lock: do advisory locking on fd
 */
static int
#ifdef __STDC__
lock(char *path)
#else
lock(path)
 char *path;
#endif
{
	int fd, val;
	struct flock flk;
	char	buf[10];

	flk.l_type = F_WRLCK;
	flk.l_start = 0;
	flk.l_whence = SEEK_SET;
	flk.l_len = 0;

	if ((fd = open(path, O_RDWR)) < 0) {
		fprintf(stderr, "%s: cannot open '%s' for read/write\n", prog, path);
		exit(1);
	}
	
	/* attempt lock of entire file */

	if (fcntl(fd, F_SETLK, &flk) < 0) {
		if (errno == EACCES || errno == EAGAIN) {
			fprintf(stderr, "%s: file '%s' being edited try later\n",
				prog, path);
			exit(0);	/* gracefully exit */
		} else {
			fprintf(stderr, "%s: error in attempting lock of '%s'\n",
				prog, path);
			exit(1);
		}
	}

	/* set close-on-exec flag for fd */

	if ((val = fcntl(fd, F_GETFD, 0)) < 0) {
		perror("getfd");
		exit(1);
	}
	val |= FD_CLOEXEC;

	if (fcntl(fd, F_SETFD, val) < 0) {
		perror("setfd");
		exit(1);
	}

	/* leave file open until we terminate: lock will be held */

	return fd;
}

static int
addvar(int n, char *name, char *value)
{
	if ((variables[n] = (Variable *)malloc(sizeof (struct variable)) ) == NULL) {
		perror("malloc");
		return 0;
	}
	variables[n]->name = name;
	variables[n++]->value = value;
	variables[n] = NULL;
	return n;
}
