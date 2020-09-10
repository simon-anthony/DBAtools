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
#include <errno.h>
/*
 * argcat: coalesce two strings separated by a blank,
 *  return pointer to malloc'ed storage.
 */
char *
#ifdef __STDC__
argcat(const char *s1, const char *s2)
#else
argcat(s1, s2)
 char *s1;
 char *s2;
#endif
{
	char	*s;

	if ((s = (char *)malloc(strlen(s1) + strlen(s2) + 2)) == NULL) {
		perror("malloc");
		exit(errno);
	}
	sprintf(s, "%s %s", s1, s2);

	return s;
}
