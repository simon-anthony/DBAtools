#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif
/* vim:syntax=c:sw=4:ts=4:
********************************************************************************
* mktemp: return a temporary file
*
********************************************************************************
* Copyright (C) 2008 Simon Anthony
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
#include <fcntl.h>

int
main(int argc, char *argv[]) {
	int		cflg = 0, dflg = 0, pflg = 0, errflg = 0;
	char	*dir = NULL, *pfx = NULL, *path;
	int		c, fd;

	while ((c = getopt(argc, argv, "cdp")) != EOF)
		switch (c) {
		case 'c':
			cflg++; 
			break;
		case 'd':
			dflg++;
			dir = optarg;
			break;
		case 'p':
			pflg++;
			pfx = optarg;
			break;
		case '?':
			errflg++;		
		}

	if (errflg) {
		fputs("usage: mktemp [-c] [-d <directory_name>] [-p prefix]\n", stderr);
		exit(2);
	}

	if ((path = tempnam(dir, pfx)) == NULL) {
		perror("tempnam");
		exit(errno);
	}
	if (cflg) {
		if ((fd = open(path, O_CREAT)) < 0) {
			perror("open");
			exit(errno);
		}
		if (close(fd) < 0) {
			perror("close");
			exit(errno);
		}
	}
	printf("%s\n", path);

	exit(0);
}
