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
#include <limits.h>
#include <libgen.h>
#include <errno.h>

#define USAGE "<path>"

static char rpath[PATH_MAX];

int 
main(int argc, char *argv[])
{
	char *prg = basename(argv[0]);

	if (argc != 2) {
		fprintf(stderr, "usage: %s %s\n", prg, USAGE);
		exit(2);
	}

	if (realpath(argv[1], rpath) == NULL) {
		perror("realpath");
		exit(errno);
	}
	printf("%s\n", rpath);

	exit(0);
}
