#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif
#include <stdio.h>
#include <unistd.h>
#include <glob.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <fnmatch.h>
#include <sys/stat.h>
#define  _RWTOP
#include "rw.h"
#if defined(_AIX) || defined(__linux__)
#include <sys/param.h>
#endif

extern char *prog;

#ifdef __STDC__
extern char *gsub(char *, char *, char *);
static int lock(char *);
static char *lower(char *);
//static char *upper(char *);
static void update(char *s, char *v, char *p);
extern char **getcontexts(char *);
extern char *param(const char *);

#else
extern char *gsub();
static int lock();
static char *lower();
//static char *upper();
static void update();
extern char **getcontexts();
#endif

static char *pathfile = PATHFILE;

static struct topstr *tptr = tops;

static char rpath[PATH_MAX+1];	/* buffer for resolved path */

static char *rpaths[ARG_MAX];	/* array of rpaths */

/*
 * rwtyp: 
 *   return code for verified type
 */
int
rwtyp(s)
 char *s;
{
	tptr = tops;			/* reset */

	while (tptr->gtop) {
#if defined (__SVR4) && defined (__sun)
		if (gmatch(s, tptr->gtop)) 
#else
		if (fnmatch(tptr->gtop, s, 0) == 0)
#endif
			return tptr->flg;
		tptr++;
	}
	return 0;
}
/*
 * rwvar: 
 *   return code for verified variable name
 */
int
rwvar(s)
 char *s;
{
	tptr = tops;			/* reset */

	while (tptr->var) {
		if (strcmp(tptr->var, s) == 0)
			return tptr->flg;
		tptr++;
	}
	return 0;
}

/*
 * rwtop:
 *   pat is an array of npat pattern strings
 *   pfile is the name of a pattern file to use otherwise NULL
 *   If pfile is NULL pat is used, otherwise pfile is used to fill pat.
 *   typ is the type of top to find.
 */
char **
rwtop(pat, npat, pfile, typ, s_systemname, flgs)
 char *pat[];
 int npat;
 char *pfile;
 int typ;
 char *s_systemname;
 int flgs;
{
	FILE	*fp;
	int		i;
	char	buf[BUFSIZ];
	char	db[BUFSIZ], top[BUFSIZ], path[BUFSIZ];
	glob_t	globbuf;
	struct topstr *t = tops;
	int		found = 0;				/* found valid TOP type? */
	int		def = ((pfile) ? 0 : 1);/* defaulted values */
	int		g = 0;					/* number of times glob() has been called */
	char    *s_systemname_lower = lower(strdup(s_systemname));    

	while (t->flg) {				/* validate type of search */
		if (typ == t->flg) {
			found++;
			break;
		}
		t++;
	}

	if (!found) {
		fprintf(stderr, "rwtop: invalid top\n");
		return NULL;
	}
	
	/*
	 * Check for explicitly set paths if no pattern file or patterns specified
	 * as argument
	 */
	if (!((RW_ISNOLOOKUP(flgs))||pfile||npat) && access(pathfile, R_OK) == 0) {
		int	n;
		if ((fp = fopen(pathfile, "r")) == NULL) {
			perror("fopen");
			exit(errno);
		}
		while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
			if (buf[0] == '#')	/* comment */
				continue;
			if ((n = sscanf(buf, "%s %s %s", db, top, path)) != 3) {
				if (n == EOF)	/* empty line */
					continue;
				if (db[0] != '#')	/* indented comment */
					fprintf(stderr, "%s: bad entry: %s", prog, (char *)buf);
				continue;
			}
			if (strcmp(db, s_systemname) == 0 && strcmp(top, t->var) == 0) {
				if (RW_ISSILENT(flgs) && (access(path, R_OK) != 0)) {
					strcpy(rpath, path);
					goto bye;
				}
				if (access(path, R_OK) != 0)
					exit(errno);
				if (RW_ISNORESOLV(flgs)) {
					strcpy(rpath, path);
					goto bye;
				}
				if (realpath(path, rpath) == NULL)
					perror("realpath");
			bye:	
				rpaths[0] = rpath;
				rpaths[1] = (char *)NULL;
				return rpaths;
			}
		}
		fclose(fp);
	}

	/*
	 * Match against patterns
	 */

	if (def)
		pfile = PATFILE;

	if (access(pfile, R_OK) == 0) {		/* read in patterns from file */
		int n;
		if ((fp = fopen(pfile, "r")) == NULL) {
			perror("fopen");
			exit(errno);
		}
		while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
			if (buf[0] == '#')	/* comment */
				continue;
			if ((n = sscanf(buf, "%s %s", top, path)) != 2) {
				if (n == EOF)	/* empty line */
					continue;
				if (top[0] != '#')	/* indented comment */
					fprintf(stderr, "%s: bad entry: %s", prog, (char *)buf);
				continue;
			}
			if (rwvar(top) == typ)	/* variable looked for */
				pat[npat++] = (char *)strdup(path);
		}
		fclose(fp);
	} else if (!def) {
		fprintf(stderr, "%s: cannot read '%s'\n", prog, pfile);
		exit(1);
	} else if (!npat) {					/* "internal" default patterns */
		struct pat	*ptr1 = defpat;

		while (ptr1->flg) {
			if (ptr1->flg == typ)
			pat[npat++] = ptr1->globpat;
			ptr1++;
		}
	}
	/* set variables for possible wordexp expansion */
	setenv("s_systemname", s_systemname, 1);
	setenv("s_systemname_lower", s_systemname_lower, 1);
	unsetenv("ORACLE_SID");
	unsetenv("TWO_TASK");

	for (i = 0; i < npat; i++) {
		char	*p, *np, *nnp, *host;
		int		n;
		char	**ctx;

		if ((p = (char *)malloc(strlen(pat[i]) + 2)) == NULL) {
			perror("malloc");
			exit(1);
		}
		strcpy(p, pat[i]);

		if ((np = strdup(param(p))) == NULL)	/* do variable substitution */
			np = p;		/* wordexp() expansion failed */

		/* Add all possible contexts */
		if (strstr(np, "{ctx}")) {
			ctx = getcontexts(s_systemname);

			while (*ctx) {
				nnp = strdup(gsub("{ctx}", *ctx, np));

				if ((n = glob(nnp, g++ > 0 ? GLOB_APPEND : 0, NULL, &globbuf)) != 0)
					if (n != GLOB_NOMATCH) {
						perror("glob");
						exit(errno);
					}
				ctx++;
				free(nnp);
			}
			goto next;
		}

		if ((n = glob(np, g++ > 0 ? GLOB_APPEND : 0, NULL, &globbuf)) != 0)
			if (n != GLOB_NOMATCH) {
				perror("glob");
				exit(errno);
			}

	next:
		free(p);
		free(np);
		if (!def)		/* pfile was supplied so */
			free(pat[i]);	/* free each strdup'ed pattern as we go */
	}

	if (globbuf.gl_pathc > 1 && typ != DATA_TOPS && !RW_ISFIRST(flgs)) {
		int	i;
		for (i = 0; i < globbuf.gl_pathc; i++) {
				fprintf(stderr, "PATH: %s\n", globbuf.gl_pathv[i]);
		}
		fprintf(stderr, "%s: - multiple paths exist for '%s'\n", prog, t->sfx);
	}

	if ((globbuf.gl_pathc == 1 && typ != DATA_TOPS) ||
		(globbuf.gl_pathc >  1 && RW_ISFIRST(flgs))) {	/* set rpath */
		if (RW_ISNORESOLV(flgs))
			strcpy(rpath, *globbuf.gl_pathv);
		else if (realpath(*globbuf.gl_pathv, rpath) == NULL) {
			perror("realpath");
			strcpy(rpath, *globbuf.gl_pathv);
		}

		if (RW_ISUPDATE(flgs))
			update(s_systemname, t->var, rpath);
		rpaths[0] = strdup(rpath);
		rpaths[1] = (char *)NULL;
	} else if (globbuf.gl_pathc >= 1 && typ == DATA_TOPS) {
		int	i;
		for (i = 0; i < globbuf.gl_pathc; i++) {
			if (RW_ISNORESOLV(flgs))
				strcpy(rpath, globbuf.gl_pathv[i]);
			else if (realpath(globbuf.gl_pathv[i], rpath) == NULL) {
				perror("realpath");
				strcpy(rpath, globbuf.gl_pathv[i]);
			}
			rpaths[i] = strdup(rpath);
		}
		rpaths[i] = (char *)NULL;
	} else
		return NULL;

	globfree(&globbuf);

	return rpaths;
}

static char *
lower(s)
 char *s;
{
	char *t = s;

	while (*s = tolower(*s))
		s++;
	
	return t;
}

/*
static char *
upper(s)
 char *s;
{
	char *t = s;

	while (*s = toupper(*s))
		s++;
	
	return t;
}
*/

static void
update(s, v, p)
 char *s;
 char *v;
 char *p;
{
	FILE	*fp, *fpt;
	char	*tmpfile;
	char	buf[BUFSIZ];
	char	db[BUFSIZ], top[BUFSIZ], path[BUFSIZ];
	int		found = 0;
#ifdef __linux
	char	template[] = "/tmp/filerwXXXXXX";
	int	fd;

	if ((fd = mkstemp(template)) == -1) {
		fprintf(stderr, "%s: cannot create temporary file name\n", prog);
		exit(errno);
	}
	tmpfile = (char *)template;

	if ((fpt = fdopen(fd, "w")) == NULL) {
#else
	if ((tmpfile = tmpnam((char *)NULL)) == NULL) {
		fprintf(stderr, "%s: cannot create temporary file name\n", prog);
		exit(1);
	}

	if ((fpt = fopen(tmpfile, "w")) == NULL) {
#endif
		fprintf(stderr, "%s: cannot open '%s' for writing\n", prog, tmpfile);
		exit(errno);
	}

	if ((fp = fdopen(lock(PATHFILE), "r")) == NULL) {
		fprintf(stderr, "%s: cannot open '%s' for reading\n", prog, PATHFILE);
		exit(errno);
	}

	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		if (buf[0] == '#')	/* comment */
			goto print;
		if (sscanf(buf, "%s %s %s", db, top, path) != 3)
			goto print;
		if (strcmp(db, s) == 0 && strcmp(top, v) == 0) {
			if (found++)
				continue;	/* ignore duplicates */
			sprintf((char *)buf, "%-4s %-8s %s\n", s, v, p);
		}
	print:
		if (fputs((char *)buf, fpt) == EOF) {
			perror("fputs");
			exit(errno);
		}
	}
	if (!found++)
		fprintf(fpt, "%-4s %-8s %s\n", s, v, p);

	fclose(fp);
	fclose(fpt);

	/* copy back */

	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	if ((fp = fopen(PATHFILE, "w")) == NULL) {
		fprintf(stderr, "%s: cannot open '%s' for writing\n", prog, PATHFILE);
		exit(1);
	}
	if ((fpt = fopen(tmpfile, "r")) == NULL) {
		fprintf(stderr, "%s: cannot open '%s' for reading\n", prog, tmpfile);
		exit(1);
	}
	while (fgets((char *)buf, BUFSIZ, fpt) != NULL)
		if (fputs((char *)buf, fp) == EOF) {
			perror("fputs");
			exit(1);
		}

	fclose(fp);
	fclose(fpt);

	if (unlink(tmpfile) != 0) {
		perror("unlink");
		exit(1);
	}

}

#include <unistd.h>
#include <fcntl.h>
/*
 * lock: do advisory locking on fd
 */
static int
lock(path)
 char *path;
{
	int fd, val;
	struct flock flk;
	char	buf[10];

	flk.l_type = F_WRLCK;
	flk.l_start = 0;
	flk.l_whence = SEEK_SET;
	flk.l_len = 0;

	if ((fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IWGRP|S_IWGRP)) < 0) {
		fprintf(stderr, "%s: cannot open '%s' for read/write\n", prog, path);
		exit(1);
	}
	
	/* attempt lock of entire file */

	if (fcntl(fd, F_SETLK, &flk) < 0) {
		if (errno == EACCES || errno == EAGAIN) {
			fprintf(stderr, "file '%s' being edited try later\n", path);
			exit(0);	/* gracefully exit */
		} else {
			fprintf(stderr, "error in attempting lock of '%s'\n", path);
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
