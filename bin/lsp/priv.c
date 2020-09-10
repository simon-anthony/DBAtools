#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif
/* Copyright (C) 2012 Simon Anthony
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
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/privileges.h>

static priv_set_t *p;

int
check_priv(priv_t id)
{
	int n;

	if ((p = privset_get(PRIV_PERMITTED, 0)) == NULL) {
		perror("privset_get");
		goto error;
	}
 
	if ((n = privset_ismember(p, id)) == 0) {
		//perror("privset_ismember");
		goto error;
	}
	
	privset_free(p);

	return n;
error:
	if (p) privset_free(p);
	return 0;
}
