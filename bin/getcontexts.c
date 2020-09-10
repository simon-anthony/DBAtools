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
#include <stdlib.h>
#include <string.h>

#define	BUFSZ		256
#define	MAXCONTEXTS		10

extern void sethost();
extern char *gethost();

static char *contexts[MAXCONTEXTS+1];
static int ncontext;

/*
 *  Get list of all possible e-Business "contexts"
 */
char **
#ifdef __STDC__
getcontexts(char *sid)
#else
getcontexts(sid)
 char *sid;
#endif
{
	int		n;
	char	*host;
	char	*ctx;

	ncontext = 0;

	/* Add all possible contexts */
	sethost();
	while ((host = gethost()) && ncontext < MAXCONTEXTS) {
		if (n = strcspn(host, "."))
			ctx = (char *)malloc(strlen(sid) + n + 2);
		else
			ctx = (char *)malloc(strlen(sid) + strlen(host) + 2);
		strcpy(ctx, sid);
		strcat(ctx, "_");
		if (n = strcspn(host, "."))
			strncat(ctx, host, n);
		else
			strcat(ctx, host);
		contexts[ncontext++] = ctx;
	}
	contexts[ncontext] = NULL;
	
	return contexts;
}
