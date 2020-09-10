#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
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
 * cmvgpkg: query/activate volume groups for SGeRAC package
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

#define PROG	"cmvgpkg"
#define USAGE	"[-v] [-a[-s]|-d[-s]] package_name"
#define HELP	"\
    -v              verbose\n\
    -a              activate volume groups in package\n\
    -s              print script to activate volume groups in package\n"

#define PATH		"PATH=/usr/sbin:/usr/bin"

#define DEACTIVATE	"vgchange -a n"
#define DISPLAY		"vgdisplay"

char *prog = PROG;
int loglevel;

#ifdef __STDC__
extern long getvar(const char *, char *, size_t, const char *);
extern char *cmviewcl(const char *nm,
	const char *e1, const char *e2, const char *e3,
	const char *v1, const char *v2, const char *v3);
extern void msg(char *node, int priority, char *fmt, ...);
int setcmd(char *[], char *);
int setargs(char *[], int, const char *);
int runcmd(char *, char *[]);
void prtcmd(char *[]);
#else
extern long getvar();
extern char *cmviewcl();
extern void msg();
int setcmd();
int setargs();
int runcmd();
void prtcmd();
#endif

char node[MAXHOSTNAMELEN];
static char *vg[ARG_MAX];
static char	activate[PATH_MAX];
static char deactivate[] = DEACTIVATE;
static char display[] = DISPLAY;

static int aflg = 0, dflg = 0, hflg = 0, vflg = 0, sflg = 0, errflg = 0;

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
	int		n;
	char	*mnp;	/* MNP name */
	int		status;
	struct	tkit tk;
	struct	tkit *tkp = NULL;
	struct	passwd *pw;
	char	*run_script;
	char	cmd[PATH_MAX];


	while ((c = getopt(argc, argv, "advhs")) != -1)
		switch (c) {
		case 'a':			/* activate */
			aflg++;
			if (dflg)
				errflg++;
			break;
		case 'd':			/* deactivate */
			dflg++;
			if (aflg)
				errflg++;
			break;
		case 's':			/* produce script */
			sflg++;
			break;
		case 'v':			/* verbose */
			loglevel++;
			vflg++;
			break;
		case 'h':			/* help */
			hflg++;
			errflg++;
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

	if (sflg) {
		if (!(aflg||dflg))			/* -a or -d mandatory if -s */
			errflg++;
		loglevel = 0;		/* no log info mixed into script */
	}

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
	if (tkp = tkit_stat(mnp))
		memcpy(&tk, tkp, sizeof(struct tkit));
	else {
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

	/* get run_script for this package */
	if ((run_script = cmviewcl("run_script",
		   "package", NULL, NULL,
		   mnp,  	  NULL, NULL)) == NULL) {
		fprintf(stderr, "%s: %s cannot get run_script for package %s\n",
			prog, pw->pw_name, mnp);
			exit(1);
	}
	if (loglevel >= 2)
		printf("run script %s\n", run_script);

	if (getvar("VGCHANGE", activate, PATH_MAX, run_script) < 0) {
		fprintf(stderr, "could not get %s\n", "VGCHANGE");
		exit(1);
	}
	
	if (aflg||dflg) {
		if (sflg) {
			n = setcmd(vg, aflg ? activate : deactivate);
			setargs(vg, n, run_script);
			prtcmd(vg);

			exit(0);
		} else {
			n = setcmd(vg, aflg ? activate : deactivate);
			setargs(vg, n, run_script);

			if (setreuid(0, -1) != 0) {
				perror("setuid");
				exit(1);
			}
		}
	} else {
		n = setcmd(vg, display);
		setargs(vg, n, run_script);
	}

	if (runcmd(vg[0], vg) != 0) {
		fprintf(stderr, "%s: failed to run command\n", prog);
		exit(1);
	}

	exit(0);
}

/*
 * setcmd: copy command string to v[] beginning at offset
 */
int
setcmd(char *v[], char *cmd)
{
	char	*a[8];  /* max option args to vgchange(1M) */
	int		i = 0, n = 0;

	bufsplit(" \t", 0, (char **)0);
	n = bufsplit(cmd, 8, a);

	for (i = 0; i < n && i < ARG_MAX; i++) {
		if (v[i])
			free(v[i]);
		v[i] = strdup(a[i]);
	}

	return i;
}

/*
 * setargs: append args to v[] beginning at offset
 */
int
setargs(char *v[], int offset, const char *path)
{
	FILE	*fp;
	char	buf[BUFSIZ];
	char	vg[BUFSIZ];
	int		n, i = offset;
	int		d;

	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "%s\n", path);
		return NULL;
	}

	while (fgets((char *)buf, BUFSIZ, fp) != NULL && i < ARG_MAX) {
		if (buf[0] == '#'||buf[0] == '\n' )
			continue;

		n = sscanf((char *)buf, "VG[%d]=\"%[^\"\n]\"", &d, vg);

		if (n != 2)
			continue;

		if (v[i])
			free(v[i]);

		v[i] = strdup((char *)vg);
#ifdef DEBUG
		fprintf(stderr, "DEBUG Assigning: VG[%d] = |%s|\n", i, v[i]);
#endif
		i++;
	}
	v[i] = (char *)0;

	return i;
}

/*
 * prtcmd: print out command array
 */
void
prtcmd(char *args[])
{
	char **p = args;

	while (**p)
		printf("%s ", *(p++));

	printf("\n");
}

/*
 * runcmd: execute command "file" with execvp
 */
int
runcmd(char *file, char *argv[])
{
	int		wstat;

	fflush(stdout);
	setbuf(stdout, NULL);

	if (putenv(PATH) != 0) {
		perror("putenv");
		return errno;
	}
	switch (fork()) {
	case -1:
		perror("fork");
		exit(1);
	case 0: /* child */
		execvp(file, argv);
		perror("exec");
		exit(1);
	}
	/* parent */

	wait(&wstat);

	if (!WIFEXITED(wstat))
			return 1;

	return 0;
}	
