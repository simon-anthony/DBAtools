#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid_x = "$Header$";
#endif
#include <stdio.h>
#include <stdlib.h>

char *getoavar(char *, char *);
char *setoavar(char *, char *, char *);

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	int n = 1;
	char * val = getoavar(argv[1], argv[2] ?  argv[2] : getenv("CONTEXT_FILE"));

	(n == 1 ? fprintf(stderr, "val is %s\n", val) : n == 2 );

	val = setoavar("s_mwatimeout", "94", argv[2] ?  argv[2] : getenv("CONTEXT_FILE"));
	exit(0);
}
