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
#include <stdlib.h>
#include <string.h>

/*
 * get context for sid
 */
static char	buf[BUFSIZ];

char *
#ifdef __STDC__
getctx(char *sid)
#else
getctx(p)
 char *sid;
#endif
{
	FILE	*fp;
	char	*cmdstr, *glnam;

	if (sid == NULL)
		return NULL;

	if ((glnam = getenv("ORACLE_GLNAM")) != NULL)
		sid = glnam;

	if ((cmdstr = (char *)malloc(strlen("/ctx -q -d -n -s ") +
			strlen(sid) + 2)) == NULL) {
		fprintf(stderr, "getctx: malloc\n");
		return NULL;
	}

	sprintf(cmdstr, "ctx -q -d -n -s %s", sid);

	if ((fp = popen(cmdstr, "r")) == NULL) {
		perror("popen");
		exit(errno);
	}

	if (fscanf(fp, "%s\n", buf) != 1) {
		fclose(fp);
		return NULL;
	}

	free(cmdstr);
	fclose(fp);

    return buf;
}
