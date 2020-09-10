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

extern void sethost();
extern char *gethost();

/*
 *  print host names
 */
int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argc[];
#endif
{
	char	*host;

	sethost();
	while (host = gethost()) {
		char *p = host;
		while (*p && *p != '.')
			putchar(*p++);
		putchar('\n');
	}

	return 0;
}
