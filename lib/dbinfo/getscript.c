#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

/*
 * Find an appsutil script in ORACLE_HOME
 */

#include <unistd.h>
#ifdef _SVR4
#include <sys/systeminfo.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UTILDIR		"/appsutil/scripts/"
#define DBSCRIPT	"addbctl.sh"
#define TNSSCRIPT	"addlnctl.sh"
#define HOSTNAME	"HOSTNAME"
#define DEFAULT		"/etc/default/oracle"

#define	BUFSZ		256

extern void sethost();
extern char *gethost();
extern char *getctx();

/*
 * getvar - extract a variable from a file
 */
long
#ifdef __STDC__
getvar(const char *var, char *s, size_t n)
#else
getvar(var, s, n)
 char *var;
 char *s;
 size_t n;
#endif
{
	FILE 	*fp;
	char	buf[BUFSIZ];
	char	lv[BUFSIZ], rv[BUFSIZ];

	if ((fp = fopen(DEFAULT, "r")) == NULL)
		return -1;

	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		if (buf[0] == '#' )
            continue;

		if (sscanf((char *)buf, "%*[ \t]%[^=]%*[=]%[^#\r\n\"]", lv, rv) != 2)
			if (sscanf((char *)buf, "%[^=]%*[=]%[^#\r\n\"]", lv, rv) != 2)
				continue;

		if (!strcmp(lv, var)) {
			strncpy(s, rv, n);
			return strlen(rv);
		}
	}
	return -1;
}

/*
 * getscript: return path of appsutil script
 *  if found-
 *    $ORACLE_HOME/appsutil/scripts/${ORACLE_SID}_`hostname`/<script>
 *  is to be used in preference to-
 *    $ORACLE_HOME/appsutil/scripts/<script>
 *  if neither exist return NULL
 */
char *
#ifdef __STDC__
getscript(const char *home, const char *sid)
#else
getscript(home, sid)
 char *home;
 char *sid;
#endif
{
	char	*host;
	char	*path, *ctx;
	int		n;

	sethost();
	while (host = gethost()) {
		if ((path = (char *)malloc(
			 strlen(home) + strlen(UTILDIR)+
			 strlen(sid) + strlen(host) + strlen(DBSCRIPT) + 4)) == NULL)
			return NULL;

		if (sprintf(path, "%s%s%s_%s/%s", home, UTILDIR, sid, host, DBSCRIPT) < 0)
			goto error;

		if (access(path, R_OK) == 0)
			return path;

		free(path);
	}
	/* Haven't got it - maybe context doesn't have hostname of network
	 * interface (hostname is an alias) */
	if (ctx = getctx(sid)) {
		if ((path = (char *)malloc(
			 strlen(home) + strlen(UTILDIR)+
			 strlen(ctx) + strlen(DBSCRIPT) + 4)) == NULL)
			return NULL;

		if (sprintf(path, "%s%s%s/%s", home, UTILDIR, ctx, DBSCRIPT) < 0)
			goto error;
		
		if (access(path, R_OK) == 0)
			return path;

		free(path);
	}

	return NULL;

error:
	if (path) free(path);
	return NULL;
}
