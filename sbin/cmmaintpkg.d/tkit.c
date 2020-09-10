#ifndef _HPUX_SOURCE
#pragma ident "$Header$"
#else
static char *svnid = "$Header$";
#endif
/* Copyright (C) 2008 Simon Anthony
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License or, (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program. If not see <http://www.gnu.org/licenses/>>
 */ 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <glob.h>
#include <unistd.h>
#include <ctype.h>
#define _TKIT
#include "tkit.h"

extern char *prog;
extern char node[];
extern int loglevel;

#ifdef __STDC__
extern long getvar(const char *, char *, size_t, const char *);
extern char *cmviewcl(const char *nm,
	const char *e1, const char *e2, const char *e3,
	const char *v1, const char *v2, const char *v3);
static char *lower(char *);
#else
extern long getvar();
extern char *cmviewcl();
static char *lower();
#endif

static char mnpdir[PATH_MAX];

/*
 * tkit_dir:
 * return configuration directory for multi-node package
 */
static char *
#ifdef __STDC__
tkit_dir(char *pkgname)
#else
tkit_dir()
 char *pkgname;
#endif
{
	char *s;


	if ((s = cmviewcl("run_script",
		"package", NULL, NULL, pkgname, NULL, NULL) ) == NULL) {
		return NULL;
	}
	return strncpy((char *)mnpdir, dirname(s), PATH_MAX);
}

/*
 * tkit_stat:
 * return status information of toolkit package given a name or a directory
 */
struct tkit *
#ifdef __STDC__
tkit_stat(char *pkg)
#else
tkit_stat()
 char *pkg;
#endif
{
	glob_t	globbuf;
	int		n;
	struct	stat statbuf;
	struct	tkit *tk = NULL;
	char	mf[BUFSIZ];	/* value of MAINTENANCE_FLAG */
	char	*s;
	char 	*path;


	if (stat(pkg, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
		path = pkg;				/* given a directory */
	} else
		path = tkit_dir(pkg);	/* given a name */

	if (chdir(path) != 0) {
		perror("chdir");
		return NULL;
	}
	if (loglevel >= 3) printf("checking %s\n", path);

	if ((n = glob(ENT_PAT, 0, NULL, &globbuf)) != 0) {
		if (n != GLOB_NOMATCH) {
			perror("glob");
			goto error;
		}
	}
	if (loglevel >= 3) printf("globbing for %s\n", ENT_PAT);

	if (globbuf.gl_pathc > 1) {
		fprintf(stderr, "%s: too many matches '%s/%s'\n", prog, path, ENT_PAT);
		goto error;
	}
	if (stat(globbuf.gl_pathv[0], &statbuf) != 0) {
		fprintf(stderr, "%s: cannot stat '%s/%s'\n", prog, path, ENT_PAT);
		goto error;
	}
	if (strcmp(globbuf.gl_pathv[0], OC_ENT) == 0)
		tk = &tkits[TKIT_OC];
	else if (strcmp(globbuf.gl_pathv[0], RAC_ENT) == 0)
		tk = &tkits[TKIT_RAC];

	if (getvar(MFLAG, mf, BUFSIZ, tk->conf) < 0) 
		tk->mflg = TK_UNKNOWN;

	if (strcmp(lower(mf), "yes") != 0)
		tk->mflg = NO;
	else if (strcmp(mf, "") == 0)
		tk->mflg = NO;
	else
		tk->mflg = YES;

	if (strcmp(path, pkg) == 0) {	/* originally given a directory name */
		/* guess that name is basename of mnp directory*/
		char *t = basename(strdup(path));
		char *name;

		if ((name = cmviewcl("name",
			"package", NULL, NULL, t, NULL, NULL) ) != NULL) {
			strcpy(tk->pkgname, t);
			if (loglevel >= 3)
				printf("correctly deduced name %s\n", tk->pkgname);
		} else {
			if (loglevel >= 3)
				printf("incorrectly deduced name %s\n",  t);
			strcpy(tk->pkgname, "unknown");
		}
		free(t);
	} else
		strcpy(tk->pkgname, pkg);
	strcpy(tk->tkit_dir, path);

error:
	globfree(&globbuf);
	return tk;
}

/*
 * tkit_getmmode:
 * return maintenance mode status of toolkit package
 */
int
#ifdef __STDC__
tkit_getmmode(struct tkit *tk)
#else
tkit_getmmode(tk)
 struct tkit *tk;
#endif
{
	char *s;
	struct stat statbuf;


	if (stat(tk->tkit_dir, &statbuf) != 0) {
		fprintf(stderr, "%s: cannot stat '%s'\n", prog, tk->tkit_dir);
		return -1;
	}
	if (!S_ISDIR(statbuf.st_mode)) {
		fprintf(stderr, "%s: '%s' not a directory\n", prog, tk->tkit_dir);
		return -1;
	}

	if ((s = (char *)malloc(strlen(tk->tkit_dir) + strlen(tk->debug) + 2)) == NULL) {
		perror("malloc");
		return -1;
	}
	sprintf(s, "%s/%s", tk->tkit_dir, tk->debug);

	if (loglevel >= 3) printf("testing for %s\n", s);

	if (stat(s, &statbuf) != 0) {
		if (errno == ENOENT) {
			if (loglevel >= 2) printf("%s not found\n", s);
			/* if RAC, check Clusterware maintenance mode */
			if (tk->type == TKIT_RAC) {
				char *s;
				char	var[PATH_MAX];
				struct tkit *tk2;

				if (getvar(OC_TKIT, var, PATH_MAX, tk->conf) < 0) {
					fprintf(stderr, "could not get %s\n", OC_TKIT);
					return TK_UNKNOWN;
				}
				if (loglevel >= 3) printf("oc_tkit_dir is %s\n", var);
				/*
				if (getvar("DEPENDENCY_NAME", var, MAX_NAME, tk->conf) < 0) {
					fprintf(stderr, "could not get depend\n");
					return TK_UNKNOWN;
				}
				printf("Dep %s\n", var);
				if ((s = cmviewcl("name",
					"package", "dependency", NULL,
					tk->pkgname, "crsp-slvm", NULL) ) == NULL) {
					return NULL;
				}
				*/
				if (tk2 = tkit_stat(var)) {
					if (loglevel >= 3) {
						printf("name is %s\n", tk2->pkgname);
						printf("debug is %s\n", tk2->debug);
						printf("type is %d\n", tk2->type);
						printf("directory is %s\n", tk2->tkit_dir);
						printf("conf is %s\n", tk2->conf);
						printf("mflg is %s\n", tk2->mflg == YES ? "YES" : "NO");
					}
					if (tkit_getmmode(tk2) == TK_ENABLED)
						return TK_ENABLED|TK_IMPLICIT;
				} else {
					fprintf(stderr, "cannot get status of %s\n", var);
					return TK_UNKNOWN;
				}
			}

			return TK_DISABLED;	/* debug file not found */
		}
		fprintf(stderr, "%s: cannot stat '%s'\n", prog, s);
		free(s);
		return -1;
	}
	if (loglevel >= 2) printf("%s found\n", s);

	free(s);
	return TK_ENABLED;	/* debug file found*/
}


/*
 * tkit_setmmode:
 * set maintenance mode status of toolkit package
 */
int
#ifdef __STDC__
tkit_setmmode(struct tkit *tk, int flg)
#else
tkit_setmmode(tk, flg)
 struct tkit *tk;
 int flg;
#endif
{
	if (flg == TK_ENABLED||flg == TK_DISABLED) {
		if (chdir(tk->tkit_dir) != 0) {
			fprintf(stderr, "%s: cannot chdir to %s on %s\n",
				prog, tk->tkit_dir, node);
			return 0;
		}
		/* access(2) and stat(2) both check real user-id so cannot be used
		if (access(tk->tkit_dir, W_OK) != 0) {
			fprintf(stderr, "%s: cannot write to %s on %s\n",
				prog, tk->tkit_dir, node);
			return 0;
		}
		*/

		if (flg == TK_ENABLED) {
			if (creat(tk->debug, S_IRUSR|S_IRGRP|S_IROTH) < 0) {
				fprintf(stderr, "%s: cannot create %s on %s [%d]\n",
					prog, tk->debug, node, errno);
			}
		} else {	/* TK_DISABLED */
			if (access(tk->debug, F_OK) != 0) {
				if (errno != ENOENT) {
					fprintf(stderr, "%s: access problem with %s on %s [%d]\n",
						prog, tk->debug, node, errno);
					return 0;
				}
			}
			if (unlink(tk->debug) != 0) {
				fprintf(stderr, "%s: failed to unlink %s on %s [%d]\n",
					prog, tk->debug, node, errno);
				return 0;
			}
		}
	} else
		return 0;

	return 1;
}

static char *
#ifdef __STDC__
lower(char *s)
#else
lower(s)
 char *s;
#endif
{
	char *t = s;

	while (*s = tolower(*s))
		s++;
	
	return t;
}
