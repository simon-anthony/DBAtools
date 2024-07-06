#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid_oa = "$Header$";
#endif
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include "ctx.h"
#ifdef linux
#include <signal.h>
#endif

static int vflg = 0, pflg = 0, eflg = 0, iflg = 0, fflg = 0, errflg = 0;

#ifdef __STDC__
extern int lock(char *);
extern void fdunlock(int);
extern int yyparse();
#else
static int lock();
extern void fdunlock();
extern int yyparse();
#endif

static Variable v;
static Variable *variable[2];


char *
#ifdef __STDC__
getoavar(const char *name, const char *path)
#else
getoavar(name, path)
 char name;
 char *path;
#endif
{
	if (path == NULL)
		return NULL;

	v.name = (char *)name;
	v.value = NULL;
	variable[0] = &v;
	variable[1] = NULL;

	ctxset(CTX_VAR, variable);

	if (freopen(path, "r", stdin) == NULL) {
		fprintf(stderr, "oavar: cannot open %s\n", path);
		return NULL;
	}

	if (!yyparse()) {
		if (ctxfound())
			return ctxget(CTX_CURVAL);
	} 
	return NULL;
}

char *
#ifdef __STDC__
setoavar(const char *name, const char *value, char *path)
#else
setoavar(name, value, path)
 char name;
 char *value;
 char *path;
#endif
{
	FILE *fp, *fpt;
	int	fd;
	char *tmppath;
	char buf[BUFSIZ];

	if (path == NULL || name == NULL || value == NULL)
		return NULL;

	if ((fp = fdopen((fd = lock(path)), "r")) == NULL) {
		fprintf(stderr, "oavar: cannot open '%s' for locking\n", path);
		return NULL;
	}
	if ((tmppath = tmpnam((char *)NULL)) == NULL) {
		fprintf(stderr, "oavar: cannot create temporary file name\n");
		return NULL;
	}
	if ((fpt = freopen(tmppath, "w", stdout)) == NULL) {
		fprintf(stderr, "oavar: cannot open '%s'\n", tmppath);
		return NULL;
	}
	if (freopen(path, "r", stdin) == NULL) {
		fprintf(stderr, "oavar: cannot open %s\n", path);
		return NULL;
	}

	v.name = (char *)name;
	v.value = (char *)value;
	variable[0] = &v;
	variable[1] = NULL;

	ctxset(CTX_VAR, variable);

	ctxsetflags(CTX_EDIT);
	ctxsetedit(1);

	if (!yyparse()) {
		if (!ctxfound())
			return NULL;
	} else
		return NULL;

	fclose(fp);
	fclose(fpt);

	/* copy back */
	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	if ((fp = fopen(path, "w")) == NULL) {
		fprintf(stderr, "oavar: cannot open '%s' for writing\n", path);
		return NULL;
	}
	if ((fpt = fopen(tmppath, "r")) == NULL) {
		fprintf(stderr, "oavar: cannot open '%s' for reading\n", tmppath);
		return NULL;
	}
	while (fgets((char *)buf, BUFSIZ, fpt) != NULL)
		if (fputs((char *)buf, fp) == EOF) {
			perror("fputs");
			return NULL;
		}
	fdunlock(fd);
	fclose(fp);
	fclose(fpt);

	if (unlink(tmppath) != 0) {
		 perror("unlink");
		 return NULL;
	}

	return (char *)value;
}
