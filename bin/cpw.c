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
#ifdef _AIX
#include <crypt.h>
#endif

#define	USAGE "[-d] <password>"

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
	int		nflg = 0, errflg = 0;
	unsigned long seed[2];
#ifdef _AIX
	char	salt[] = "{SMD5}........$";
#else
	char	salt[] = "$1$........";
#endif
	const char *const seedchars = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	char	*password;


	prog = strdup(basename(argv[0]));

	while ((c = getopt(argc, argv, "n")) != EOF)
		switch (c) {
		case 'n':		/* display members */
			nflg++;
			break;
		case '?':
			errflg++;
		}

	if (argc - optind != 0)
		errflg++;

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}
	if (!nflg) {
		strcpy(salt, "g11jThdv7/0IQ");
		password = crypt(getpass("Password:"), salt);
		if (strcmp(salt, password) == 0) 
			exit(0);
		exit(1);
	}

	seed[0] = time(NULL);
	seed[1] = getpid() ^ (seed[0] >> 14 & 0x30000);

	for (i = 0; i < 8; i++)
		salt[6+i] = seedchars[(seed[i/5] >> (i%5)*6) & 0x3f];

	password = crypt(getpass("Password:"), salt);

	puts(password);

	exit(0);
}
