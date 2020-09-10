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
#include <errno.h>
#include <varargs.h>
#include <unistd.h>
#include <syslog.h>
#include <limits.h>

extern char *prog;
//extern char node[];
/*
 * LOG_EMERG    A panic condition.  This is normally broadcast to all users.
 *
 * LOG_ALERT    A condition that should be corrected immediately, such as a
 *              corrupted system database.
 *
 * LOG_CRIT     Critical conditions, such as hard device errors.
 *
 * LOG_ERR      Errors.
 *
 * LOG_WARNING  Warning messages.
 *
 * LOG_NOTICE   Conditions that are not error conditions, but should possibly
 *              be handled specially.
 *
 * LOG_INFO     Informational messages.
 *
 * LOG_DEBUG    Messages that contain information normally of use only when
 *              debugging a program.
 */

static char buf[ARG_MAX];
/*
 * log:
 * log message
 */
void
msg(node, pri, fmt, va_alist)
char *node;
int pri;
char *fmt;
va_dcl
{
	va_list	args;
	FILE	*fp = (pri == LOG_INFO || pri == LOG_WARNING || pri == LOG_NOTICE ?
		stdout : stderr);

	va_start(args);

	vsnprintf(buf, ARG_MAX, fmt, args);

	/* print message to user */
	if (fp == stderr)					/* prepend program name for errors */
		fprintf(stderr, "%s: ", prog);
	fprintf(fp, "%s", (char *)buf);
	if (node)
		fprintf(fp, " on node %s", node);
	fprintf(fp, "\n");

	/* print message to syslog() */
	if (pri != LOG_INFO) {
		openlog(prog, LOG_PID, LOG_USER);
		syslog(pri, (char *)buf);
		closelog();
	}

	va_end(args);
}
