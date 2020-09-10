#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
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

/*
 * cmmaintpkg: set maintenance mode on SGeRAC package
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <pwd.h>
#include <syslog.h>
#include "tkit.h"

#define PROG	"cmmaintpkg"
#define USAGE	"[-v] [-e|-d] package_name"
#define HELP	"\
    -v              verbose\n\
    -e              enable maintenance mode\n\
    -d              disable maintenance mode\n"

char *prog = PROG;
int loglevel;

#ifdef __STDC__
extern long getvar(const char *, char *, size_t, const char *);
extern char *cmviewcl(const char *nm,
	const char *e1, const char *e2, const char *e3,
	const char *v1, const char *v2, const char *v3);
extern void msg(char *node, int priority, char *fmt, ...);

#else
extern long getvar();
extern char *cmviewcl();
extern void msg();
#endif

char node[MAXHOSTNAMELEN];

static int cflg = 0, sflg = 0, eflg = 0, dflg = 0, hflg = 0, vflg = 0,
		errflg = 0;

#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	extern	char *optarg;
	extern	int optind;
	int		c;
	char	*mnp;	/* MNP name */
	int		status;
	struct	tkit tk;
	struct	tkit *tkp = NULL;
	struct	passwd *pw;


	while ((c = getopt(argc, argv, "edhv")) != -1)
		switch (c) {
		case 'e':
			eflg++;
			break;
		case 'd':
			dflg++;
			break;
		case 'h':
			hflg++;
			errflg++;
			break;
		case 'v':
			loglevel++;
			vflg++;
			break;
		case '?':
			errflg++;
		}

	if (geteuid() != 0) {
		fprintf(stderr, "%s: you must be root to run this command\n", prog);
		exit(1);
	}

	if (argc - optind == 0)		/* need additional args */
		errflg++;

	mnp = argv[optind];

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		if (hflg) fprintf(stderr, "%s\n", HELP);
		exit(2);
	}

	if (gethostname((char *)node, MAXHOSTNAMELEN) < 0) {
		perror("gethostname");
		exit(errno);
	}

	if ((pw = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "%s: cannot determine username\n", prog);
		perror("getuid");
		exit(errno);
	}

	/* check invoker is an admin for this package */
	if (strcmp(pw->pw_name, "root") != 0) {
		if ((cmviewcl("name",
			   "package", "authorized_host",     "user",
			   mnp,   "cluster_member_node", pw->pw_name)) == NULL) {
			fprintf(stderr, "%s: %s is not an authorised admin of package %s\n",
				prog, pw->pw_name, mnp);
			exit(1);
		}
	}

	if (loglevel >= 2)
		printf("admin name is %s\n", pw->pw_name);

	/* determine status information from the package */

	if (tkp = tkit_stat(mnp)) {
		memcpy(&tk, tkp, sizeof(struct tkit));
		if (loglevel >= 2) {
			printf("name is %s\n", tk.pkgname);
			printf("debug is %s\n", tk.debug);
			printf("type is %d\n", tk.type);
			printf("directory is %s\n", tk.tkit_dir);
			printf("conf is %s\n", tk.conf);
			printf("mflg is %s\n", tk.mflg == YES ? "YES" : "NO");
		}
	} else {
		msg(NULL, LOG_ERR, "cannot access package %s", mnp);
		exit(1);
	}

	if ((status = tkit_getmmode(&tk)) == TK_UNKNOWN) {
		msg(NULL, LOG_ERR, "cannot determine toolkit %s", node);
		exit(1);
	}

	if (TK_ISNOTIMPL(status)) {
		msg((char *)node, LOG_ERR,
			"maintenance mode not implemented for %s", mnp);
		exit(1);
	}
	if (!(eflg|dflg)) { /* return status */
		if (TK_ISDISABLED(status))
			msg((char *)node, LOG_INFO,
				"Maintenance mode for %s is DISABLED", mnp);
		if (TK_ISENABLED(status))
			msg((char *)node, LOG_INFO,
				"Maintenance mode for %s is ENABLED%s", mnp,
				TK_ISIMPLICIT(status) ?  " (implicitly)" : "");
		exit(0);
	}

	if (eflg) {
		if (TK_ISIMPLICIT(status)) {
			msg((char *)node, LOG_WARNING,
				"Maintenance mode implicitly enabled for %s", mnp);
			// enable it anyway? - no, be consistent with cm* commands
			exit(0);
		}
		if (TK_ISENABLED(status)) {
			msg((char *)node, LOG_NOTICE,
				"Maintenance mode already enabled for %s", mnp);
			exit(0);
		}
		if (tkit_setmmode(&tk, TK_ENABLED))
			msg((char *)node, LOG_NOTICE,
				"Maintenance mode enabled for %s", mnp);
		else {
			msg((char *)node, LOG_ERR,
				"Failed to enable maintenance mode for %s", mnp);
			exit(1);
		}
	} else if (dflg) {
		if (TK_ISIMPLICIT(status)) {
			msg((char *)node, LOG_WARNING,
				"Maintenance mode implicitly enabled for %s", mnp);
			// disable it anyway? - no, be consistent with cm* commands
			exit(0);
		}
		if (TK_ISDISABLED(status)) {
			msg((char *)node, LOG_NOTICE,
				"Maintenance mode already disabled for %s", mnp);
			exit(0);
		}
		if (tkit_setmmode(&tk, TK_DISABLED))
			msg((char *)node, LOG_NOTICE,
				"Maintenance mode disabled for %s", mnp);
		else {
			msg((char *)node, LOG_ERR,
				"Failed to disable maintenance mode for %s", mnp);
			exit(1);
		}
	}
	exit(0);
}
