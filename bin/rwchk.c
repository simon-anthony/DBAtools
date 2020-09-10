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
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include "rw.h"

extern char *prog;

#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * chk: wrapper to check file ownership for oracle and appl
 */
static int
#ifdef __STDC__
chk(char *path, char *pattern, int chksuid)
#else
chk(path, pattern, chksuid)
 char *path;
 char *pattern;
 int  chksuid;
#endif
{
	glob_t		globbuf;
	int			i;
	static int	n = 0;
	struct stat	statbuf;

	if (chdir(path) != 0) {
		perror("chdir");
		exit(errno);
	}

	if ((n = glob(pattern, 0, NULL, &globbuf)) != 0) {
		if (n != GLOB_NOMATCH) {
			perror("glob");
			exit(errno);
		}
	}

	for (i = 0; i < globbuf.gl_pathc; i++) {
		if (stat(globbuf.gl_pathv[i], &statbuf) != 0) {
			fprintf(stderr, "%s: cannot stat '%s/%s'\n", prog, path, pattern);
			continue;
		}
		if (!chksuid)
			return statbuf.st_uid;

		if (S_ISREG(statbuf.st_mode) && (statbuf.st_mode & 0xF00 & S_ISUID))
			return statbuf.st_uid;

	}

	return -1;
}

#define ORA_FILE	"*/bin/oracle"
#define APPL_FILE	"*/admin/topfile.txt"

/*
 * rwchk: return uid owner found at path, -1 otherwise
 */
int
#ifdef __STDC__
rwchk(int type, char *path)
#else
rwchk(type, path)
 int  type;
 char *path;
#endif
{
	if (type == ORA_TOP || type == DB_TOP)
		return chk(path, ORA_FILE, 1);

	if (type == APPL_TOP)
		return chk(path, APPL_FILE, 0);

	return -1;
}
