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
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "rw.h"
#include <pwd.h>

#define	USAGE "[-1] [-v] [-i|-f<file>|-p<pattern>...|[-d<path>][-o<path>]] <s_systemname>"

#define EXE	"*/bin/oracle" 		 /* setuid oracle executable file ./?/bin */
#define OBJ	"*/rdbms/lib/xaonsl.o" /* object file present ./?/rdbms/lib */

/* ORACLE_HOME type */
#define DB_OH	 	0
#define TOOLS_OH	1

char *prog;

static struct passwd *pw;
static char *ora_top = NULL; 
static char *db_top = NULL; 

static int vflg = 0;

#ifdef __STDC__
static uid_t search(char *, int);
static uid_t findowner(char *, int);
#else
static uid_t search();
static uid_t findowner();
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
	int		c, i;
	extern char	*optarg;
	extern int	optind;
	char	*pat[MAXPAT];
	int		npat = 0;
	char	*dir, *s_systemname;
	char	*pfile = (char *)NULL;
	char	**path;
	int		flgs = 0;
	int		type;
	uid_t	uid = -1;
	int		iflg = 0, fflg = 0, pflg = 0,
			oflg = 0, dflg = 0, errflg = 0;
	int		oh = DB_OH;	/* ORACLE_HOME type */


	prog = strdup(basename(argv[0]));

	while ((c = getopt(argc, argv, "1if:p:d:o:v")) != EOF)
		switch (c) {
		case '1':		/* restrict search to first */
			flgs |= RW_FIRST;
			break;
		case 'i':		/* ignore lookup file */
			if (pflg||fflg||oflg||dflg)
				errflg++;
			iflg++;
			flgs |= RW_NOLOOKUP;
			break;
		case 'f':
			if (pflg||iflg||oflg||dflg)
				errflg++;
			fflg++;
			pfile = optarg;
			break;
		case 'p':
			if (fflg||iflg||oflg||dflg)
				errflg++;
			pflg++;
			if (npat < MAXPAT)
				pat[npat++] = optarg;
			break;
		case 'd':		/* DB_TOP specified */
			if (fflg||iflg||pflg)
				errflg++;
			dflg++;
			db_top = optarg;
			break;
		case 'o':		/* ORA_TOP specified */
			if (fflg||iflg||pflg)
				errflg++;
			oflg++;
			ora_top = optarg;
			break;
		case 'v':		/* verbose */
			vflg++;
			break;
		case '?':
			errflg++;
		}

	if (argc - optind != 1)
		errflg++;

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}
	s_systemname = argv[optind];

	/* search for Oracle executable */

	if (db_top) {
		path = &db_top;

		if (access(*path, R_OK) != 0) {
			fprintf(stderr, "%s: cannot access '%s'\n", prog, *path);
			exit(errno);
		}
	} else
		path = rwtop(pat, npat, pfile, DB_TOP, s_systemname, flgs);

	if (findowner(*path, oh) >= 0)
		goto found;

	/* search for object file */
	oh = TOOLS_OH;

	if (ora_top) {
		path = &ora_top;
		if (access(*path, R_OK) != 0) {
			fprintf(stderr, "%s: cannot access '%s'\n", prog, *path);
			exit(errno);
		}
	} else
		path = rwtop(pat, npat, pfile, ORA_TOP, s_systemname, flgs);

	if (findowner(*path, oh) >= 0)
		goto found;

	exit(1);

found:
	exit(0);
}

static uid_t
findowner(char *path, int tools)
{
	uid_t	uid;

	if ((uid = search(path, tools)) > 0) {
		if (pw = getpwuid(uid))
			printf("%s\n", pw->pw_name);
		else
			printf("%d\n", uid);
		return uid;
	}
	return -1;
}

#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * search: return uid of oracle executable (which is setuid) found at path
 * -1 otherwise
 */
static uid_t
#ifdef __STDC__
search(char *path, int tools)
#else
search(path, tools)
 char *path;
 int tools;
#endif
{
	char		*pattern = EXE;
	glob_t		globbuf;
	int			i;
	static int	n = 0;
	struct stat	statbuf;

	if (tools)
		pattern = OBJ;

	if (chdir(path) != 0) {
		perror("chdir");
		exit(errno);
	}

	if (vflg) fprintf(stderr, "%s: searching %s/%s\n", prog, path, pattern);

	if ((n = glob(pattern, 0, NULL, &globbuf)) != 0) {
		if (n != GLOB_NOMATCH) {
			perror("glob");
			exit(errno);
		}
	}

	for (i = 0; i < globbuf.gl_pathc; i++) {
		if (vflg) fprintf(stderr, "%s: checking %s\n", prog, globbuf.gl_pathv[i]);
		if (stat(globbuf.gl_pathv[i], &statbuf) != 0) {
			fprintf(stderr, "%s: cannot stat '%s/%s'\n", prog, path, pattern);
			continue;
		}
		if (S_ISREG(statbuf.st_mode) && (tools ? 1 : (statbuf.st_mode & 0xF00 & S_ISUID)))
			return statbuf.st_uid;

	}

	return -1;
}
