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
#include <sys/stat.h>
#include <fnmatch.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif
#include "dbinfo.h"

struct cmdstr *
#ifdef __STDC__
setcmd(struct cmdstr *p, const char *name)
#else
setcmd(p, name)
 struct cmdstr *p;
 char *name;
#endif
{
	struct stat	statbuf;

	if (!fnmatch("*/appsutil/*", name, 0)) { 	/* command is a script path */
		if (snprintf(p->path, PATH_MAX, "%s", name) < 0) {
			perror("snprintf");
			return NULL;
		}
	} else {
		if (snprintf(p->path, PATH_MAX, "%s/bin/%s", p->oracle_home, name) <0) {
			perror("snprintf");
			return NULL;
		}
	}
	if (stat(p->path, &statbuf) < 0) {
		if (errno == ENOENT) {
			return NULL;
		}
		perror("stat");
		return NULL;
	}

	return p;
}
