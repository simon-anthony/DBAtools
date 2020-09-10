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
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <sys/pstat.h>
#include <pwd.h>
#include <sys/privileges.h>

#define	USAGE	"[-t] [-n] [-x] [-p port] [-u userlist] [-U userlist]"

static char *prog;

#ifdef __STDC__
extern void getports(int);
int check_priv(priv_t);
#else
extern void getports();
int check_priv();
#endif

extern int terse;
extern int extended;
extern int noresolve;
extern int nofqdn;

static int euidlist[BUFSIZ];
static int neuids = 0;

static int uidlist[BUFSIZ];
static int nuids = 0;

static int nflg = 0, hflg = 0, tflg = 0, xflg = 0, pflg = 0, uflg = 0, Uflg = 0, errflg = 0;

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int    argc;
 char   *argv[];
#endif
{
	int			c;
	extern char	*optarg;
	extern int	optind;
	struct tm	*tp;
	char		*buf[BUFSIZ];
	int			port = 0;
	int			n;
	char		*arg, *args;
	struct passwd *pwd;

	prog = basename(argv[0]);

	while ((c = getopt(argc, argv, "hnxtp:u:U:?")) != EOF)
		switch (c) {
		case 'h':
			hflg++;
			nofqdn = 1;
			break;
		case 'n':
			nflg++;
			noresolve = 1;
			break;
		case 't':
			tflg++;
			terse = 1;
			break;
		case 'x':
			xflg++;
			extended = 1;
			break;
		case 'p':
			port = atoi(optarg);
			pflg++;
			break;
		case 'u':              /* csv list of effective user ids */
		case 'U':              /* csv list of real user ids */
			(c == 'u') ? uflg++ : Uflg++;
			args = optarg; /* parse session euids */

			while ((arg = strtok(args, ", ")) != NULL && ((c == 'u')  ? neuids < BUFSIZ : nuids < BUFSIZ)) {
				if (n = atoi(arg)) {
					(c == 'u') ? (euidlist[neuids++] = n)
								: (uidlist[nuids++] = n);
				} else {
					if ((pwd = getpwnam(arg)) != NULL) {
						(c == 'u') ? (euidlist[neuids++] = pwd->pw_uid)
									: (uidlist[nuids++] = pwd->pw_uid);
					} else {
						fprintf(stderr, "%s: unknown user %s\n", prog, arg);
						exit(1);
					}
				}
				args = NULL;
			}
			break;
		case '?':
			errflg++;
		}

	if (argc - optind != 0)
		errflg++;

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}

	if (!((uflg == 1 && Uflg == 0 && euidlist[0] == geteuid()) ||
		  (Uflg == 1 && uflg == 0 &&  uidlist[0] ==  getuid()))) {
		if (check_priv(PRIV_OWNER) == 0) 
			fprintf(stderr, "%s: warning - no OWNER privilege, only your processes displayed\n", prog);
	}

	getports(port);

	exit(0);
}

int
#ifdef __STDC__
ckuser(struct pst_status *pstp)
#else
ckuser(pstp)
 struct pst_status *pstp;
#endif
{
	int		i, found = 0;

	if (!(uflg||Uflg))
		return 1;
	if (uflg) {            /* effective user list */
		found = 0;
		for (i = 0; i < neuids && i < BUFSIZ; i++)
			if (pstp->pst_euid == euidlist[i]) {
				found++;
				break;
			}
	}
	if (Uflg) {            /* real user list */
		found = 0;
		for (i = 0; i < nuids && i < BUFSIZ; i++)
			if (pstp->pst_uid == uidlist[i]) {
				found++;
				break;
			}
	}
	return found;
}
