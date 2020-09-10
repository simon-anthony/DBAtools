#ifndef _HPUX_SOURCE
#pragma ident "$Header$"
#else
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
#include <string.h>
/*
 * getvar - extract a variable from a file
 */
long
#ifdef __STDC__
getvar(const char *var, char *s, size_t n, const char *path)
#else
getvar(var, s, n, path)
 char *var;
 char *s;
 size_t n;
 char *path;
#endif
{
	FILE	*fp;
	char	buf[BUFSIZ];
	char	lv[BUFSIZ], rv[BUFSIZ];
	int		ns;
	
	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "%s\n", path);
		perror("fopen");
		return NULL;
	}
	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		if (buf[0] == '#' || buf[0] == '\n')
			continue;

		/* check for quoted r-value */
		ns = sscanf((char *)buf, "%[^=]%*[=]%*[\"\']%[^#\r\n\"\']%*[\"\']", lv, rv);
		if (ns != 2) { 
			/* check for unquoted r-value */
			ns = sscanf((char *)buf, "%[^=]%*[=]%[^#\r\n]", lv, rv);
			if (ns != 2)
				continue;
		}

		if (!strcmp(lv, var)) {
			if (ns == 1) {
				strcpy(s, "");
				return 0;
			}
			strncpy(s, rv, n);
			return strlen(rv);
		}
	}
	return -1;
}
