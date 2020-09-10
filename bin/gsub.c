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
#include <regex.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static char buf[ARG_MAX];

static regex_t		re;							/* compile struct */
/*static regmatch_t	pm[_REG_SUBEXP_MAX + 1];	 array of match structs */
static regmatch_t	pm;							/* match struct */


/*
 * gsub():
 *  Substitutes "repl" for the first occurrence of the extended regular
 *  expression "ere" in the string "in". Returns a pointer to static storage
 *  which is overwritten on each call.
 */
char *
gsub(ere, repl, in)
 char *ere;
 char *repl;
 char *in;
{
	int		n, sub = 0, off = 0;
	int		i = 0;
	char	*p = in;

	if (!in)
		return NULL;

	if (strlen(in) == 0)
		return in;
	
	if ((n = regcomp(&re, ere, 0)) != 0)
		goto error;

	if ((n = regexec(&re, &in[0], 1, &pm, 0)) == 0) {

		while (n == 0) {

			p = in + off;

			while ((p < in + pm.rm_so) && i < ARG_MAX)
				buf[i++] = *p++;

			p = repl;

			while ((buf[i++] = *p++) && i < ARG_MAX)  /* copy in replacement */
				;
			i--;    /* don't copy \0 */


			off = pm.rm_eo;

			n = regexec(&re, &in[pm.rm_eo], 1, &pm, REG_NOTBOL);

			pm.rm_so += off;
			pm.rm_eo += off;

		}
		p = in + off;		/* trailing part */
		while ((buf[i++] = *p++) && i < ARG_MAX)
			;
		buf[i] = '\0';

		p = (char *) buf;
	} else
		p = in;


	regfree(&re);
	return p;
		
error:
	regerror(n, &re, buf, sizeof buf);
	regfree(&re);

	return NULL;
}
