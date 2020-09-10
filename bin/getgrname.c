#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
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
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <grp.h>

#define	USAGE "[-m] <groupname>"

#define SEP	" "

char *prog;

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	int		c, i;
	extern char	*optarg;
	extern int	optind;
	int		mflg = 0, errflg = 0;
	struct group *grp;

	prog = strdup(basename(argv[0]));

	while ((c = getopt(argc, argv, "m")) != EOF)
		switch (c) {
		case 'm':		/* display members */
			mflg++;
			break;
		case '?':
			errflg++;
		}

	if (argc - optind != 1)
		errflg++;

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}

	setgrent();

	if ((grp = getgrnam(argv[optind])) == NULL) {
		fprintf(stderr, "no such group\n");
		exit(1);
	}
	//printf("%s", grp->gr_name);

	if (mflg) {
		char **p = grp->gr_mem;
		char sep[2] = "";

		while (*p) {
			printf("%s%s", sep, *(p++));
			strcpy(sep, SEP);
		}

	}
	printf("\n");

	exit(0);
}
