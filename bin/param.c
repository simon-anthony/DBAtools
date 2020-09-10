#ifdef __SUN__
#pragma ident "$Header: DBAtools/trunk/bin/mktemp.c 89 2017-08-11 11:47:22Z xanthos $"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header: DBAtools/trunk/bin/mktemp.c 89 2017-08-11 11:47:22Z xanthos $")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header: DBAtools/trunk/bin/mktemp.c 89 2017-08-11 11:47:22Z xanthos $";
#endif
/* vim:syntax=c:sw=4:ts=4:
********************************************************************************
* param: perform parameter substitution
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
#include <wordexp.h>
static wordexp_t result;

char *
param(const char *s)
{

	if (wordexp(s, &result, WRDE_NOCMD|WRDE_REUSE)) {
		wordfree(&result);
		return NULL;
	}

	return result.we_wordv[0];
}
