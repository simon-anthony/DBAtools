#ifdef _HPUX_SOURCE
#include <sys/mpctl.h>
#else
#pragma ident "$Header$"
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
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#define CMVIEWCL	"/usr/sbin/cmviewcl -v -f line -s config"
#define ELEM3		"^%s:%s|%s:%s|%s:%s|%s=\\(.*\\)$"
#define ELEM2		"^%s:%s|%s:%s|%s=\\(.*\\)$"
#define ELEM1		"^%s:%s|%s=\\(.*\\)$"

static regex_t		re;							/* compile struct */
static regmatch_t pm[_REG_SUBEXP_MAX + 1];      /* array of match structs */
static char	buf[BUFSIZ];						/* result in static storage */
/* 
 * cmviewcl:
 * Return value from matching name in name=value pairs from cmviewcl
 * qualified by elements (eN) and their respective values (vN).
 * Where no element/value pairs are used to qualify NULL is used.
 *
 * Examples:
 *	char *value;
 *
 *	value = cmviewcl("run_script",
 *		"package", NULL, NULL,
 *		"crsp-slvm", NULL, NULL);
 *	printf("[1] '%s'\n", value);
 *
 *	value = cmviewcl("name",
 *		"package", "node", NULL,
 *		"racp-dino", "ukgtjd36", NULL);
 *	printf("[2] '%s'\n", value);
 *
 *	value = cmviewcl("name",
 *		"package", "authorized_host", "user",
 *		"racp-dino", "cluster_member_node", "oradino");
 *	printf("[3] '%s'\n", value);
 */
char *
cmviewcl(const char *name,
	const char *e1, const char *e2, const char *e3,
	const char *v1, const char *v2, const char *v3)
{
	FILE	*fp;
	char	*ere;
	int 	n;

	if (! name)	/* name required to get value */
		return NULL;
	if (! e1)	/* need at least one element */
		return NULL;

	if (e3) {
		if ((ere = (char *)malloc(strlen(name)+
				strlen(e1) + strlen(v1) +
				strlen(e2) + strlen(v2) +
				strlen(e3) + strlen(v3) + strlen(ELEM3))) == NULL) {
			perror("malloc");
			return NULL;
		}
		sprintf(ere, ELEM3, e1, v1, e2, v2, e3, v3, name);
	} else if (e2) {
		if ((ere = (char *)malloc(strlen(name)+
				strlen(e1) + strlen(v1) +
				strlen(e2) + strlen(v2) + strlen(ELEM2))) == NULL) {
			perror("malloc");
			return NULL;
		}
		sprintf(ere, ELEM2, e1, v1, e2, v2, name);
	} else if (e1) {
		if ((ere = (char *)malloc(strlen(name)+
				strlen(e1) + strlen(v1) + strlen(ELEM1))) == NULL) {
			perror("malloc");
			return NULL;
		}
		sprintf(ere, ELEM1, e1, v1, name);
	}

	if ((n = regcomp(&re, ere, 0) != 0))
		goto error;

	if ((fp = popen(CMVIEWCL, "r")) == NULL) {
		perror("popen");
		return NULL;
	}

	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		if ((n = regexec(&re, buf, (size_t) 2, pm, 0)) == 0) {

			free(ere);
			buf[pm[1].rm_eo-1] = '\0';	/* blat newline */
			return &buf[pm[1].rm_so];
		}
	}
error:
	regerror(n, &re, buf, sizeof buf);
	regfree(&re);
	free(ere);

	return NULL;
}
