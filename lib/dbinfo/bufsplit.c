/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

/*
 *	Split buffer into fields delimited by tabs and newlines.
 *	Fill pointer array with pointers to fields.
 * 	Return the number of fields assigned to the array[].
 *	The remainder of the array elements point to the end of the buffer.
 *
 *      Note:
 *	The delimiters are changed to null-bytes in the buffer and array of
 *	pointers is only valid while the buffer is intact.
 */

#ifndef __IBMC__
#ifndef __linux__
#pragma weak bufsplit = _bufsplit

#include "gen_synonyms.h"
#endif /* __linux__ */
#endif /* __IBMC__ */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#ifdef _REENTRANT
#include <thread.h>
#endif

#ifndef _REENTRANT
static char *bsplitchar = "\t\n";	/* characters that separate fields */
#endif

#ifdef _REENTRANT
static char **
_get_bsplitchar(thread_key_t *key)
{
	static char **strp = NULL;
	static char *init_bsplitchar = "\t\n";

	if (thr_getspecific(*key, (void **)&strp) != 0) {
		if (thr_keycreate(key, free) != 0) {
			return (NULL);
		}
	}
	if (!strp) {
		if (thr_setspecific(*key, (void *)(strp = malloc(
			sizeof (char *)))) != 0) {
			if (strp)
				(void) free(strp);
			return (NULL);
		}
		*strp = init_bsplitchar;
	}
	return (strp);
}
#endif /* _REENTRANT */

size_t
bufsplit(char *buf, size_t dim, char **array)
{
#ifdef _REENTRANT
	static thread_key_t key = 0;
	char  **bsplitchar = _get_bsplitchar(&key);
#define	bsplitchar (*bsplitchar)
#endif /* _REENTRANT */

	unsigned numsplit;
	int	i;

	if (!buf)
		return (0);
	if (!dim ^ !array)
		return (0);
	if (buf && !dim && !array) {
		bsplitchar = buf;
		return (1);
	}
	numsplit = 0;
	while (numsplit < dim) {
		array[numsplit] = buf;
		numsplit++;
		buf = strpbrk(buf, bsplitchar);
		if (buf)
			*(buf++) = '\0';
		else
			break;
		if (*buf == '\0') {
			break;
		}
	}
	buf = strrchr(array[numsplit-1], '\0');
	for (i = numsplit; i < dim; i++)
		array[i] = buf;
	return (numsplit);
}
