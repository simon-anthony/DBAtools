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
#include <libgen.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "rw.h"

#define	USAGE "[-1] [-v] [-sun] [-i|-f<file>|-p<pattern>...] {ora|comn|appl|db|arch|inst|data} <s_systemname>"

char *prog;

int
main(argc, argv)
 int argc;
 char *argv[];
{
	int		c, i;
	extern char	*optarg;
	extern int	optind;
	char	*pat[MAXPAT];
	int		npat = 0;
	char	*dir, *s_systemname;
	char	*pfile = (char *)NULL;
	char	**path;
	int		flgs = 0;
	int		type;
	int		oneflg = 0, aflg = 0, sflg = 0, iflg = 0, uflg = 0, nflg = 0,
			fflg = 0, pflg = 0, vflg = 0, errflg = 0;


	prog = strdup(basename(argv[0]));

	while ((c = getopt(argc, argv, "1vsinuf:p:")) != EOF)
		switch (c) {
		case '1':		/* restrict search to first */
			oneflg++;
			flgs |= RW_FIRST;
			break;
		case 'v':		/* validate ORA_TOP, DB_TOP or APPL_TOP */
			vflg++;
			break;
		case 's':		/* for explicit lookups - return non-existent paths */
			sflg++;
			flgs |= RW_SILENT;
			break;
		case 'u':		/* update path file */
			uflg++;
			flgs |= RW_UPDATE;
			break;
		case 'n':		/* don't try to resolve paths */
			nflg++;
			flgs |= RW_NORESOLV;
			break;
		case 'i':		/* ignore lookup file */
			if (pflg||fflg)
				errflg++;
			iflg++;
			flgs |= RW_NOLOOKUP;
			break;
		case 'f':		/* file containing patterns */
			if (pflg||iflg)
				errflg++;
			fflg++;
			pfile = optarg;
			break;
		case 'p':		/* patterns */
			if (fflg||iflg)
				errflg++;
			pflg++;
			if (npat < MAXPAT)
				pat[npat++] = optarg;
			break;
		case '?':
			errflg++;
		}

	if (argc - optind != 2)
		errflg++;

	if (argc - optind > 1) {
		dir = argv[optind];

		if ((type = rwtyp(dir)) == 0)
			errflg++;
	}

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}
	s_systemname = argv[optind + 1];

	if (path = rwtop(pat, npat, pfile, type, s_systemname, flgs)) {
		if (type != DATA_TOPS) {
			if (!oneflg && !vflg) {
				printf("%s\n", *path);
			} else if (vflg && (type == ORA_TOP ||
								type == DB_TOP ||
								type == APPL_TOP)) {
				while (*path) {
					if (rwchk(type, *path)) {
						printf("%s\n", *path);
						exit(0);
					}
					*(path++);
				}
				exit(1);
			} else
				printf("%s\n", *path);
		} else {
			while (*path)
				printf("%s\n", *(path++));
		}
		exit(0);
	}

	exit(1);
}
