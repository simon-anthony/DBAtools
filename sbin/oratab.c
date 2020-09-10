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
 * oratab modifier
 */

#if defined (_HPUX_SOURCE) || defined (linux)
#define ORATAB	"/etc/oratab"
#else
#define ORATAB	"/var/opt/oracle/oratab"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#define _ORATAB
#define USAGE "{[-y|-n|-d|-p|-h <ORACLE_HOME>] <ORACLE_SID> | -l[-p][-y|n]}"

char *prog;

#ifdef __STDC__
static int lock(char *);
#else
static int lock();
#endif

int
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
	struct	stat statbuf;
	char	oracle_sid[BUFSIZ], oracle_home[BUFSIZ], buf[BUFSIZ];
	char	start, ch;
	char	*sid, *home, *tmpfile;
	int		yflg = 0, nflg = 0, dflg = 0, pflg = 0, hflg = 0, lflg = 0,
			errflg = 0, found = 0;
	int		c;
	int		fd, fdt;
	FILE	*fp, *fpt;
#ifdef __linux
	char	template[] = "/tmp/filerwXXXXXX";

	if ((fd = mkstemp(template)) == -1) {
		fprintf(stderr, "%s: cannot create temporary file name\n", prog);
		exit(errno);
	}
	tmpfile = (char *)template;
#else
	if ((tmpfile = tmpnam((char *)NULL)) == NULL) {
		fprintf(stderr, "%s: cannot create temporary file name\n", prog);
		exit(1);
	}
#endif

	prog = basename(argv[0]);

	while ((c = getopt(argc, argv, "lyYnNdh:p")) != -1)
		switch (c) {
		case 'l':		/* list all SIDs */
			lflg++;
			if (dflg||hflg)
				errflg++;
			break;
		case 'y':		/* switch start flag to 'Y' */
		case 'Y':
			yflg++;
			ch = 'Y';
			if (lflg) {
				if (nflg||dflg||hflg)
					errflg++;
			} else if (nflg||dflg||hflg||pflg)
					errflg++;
			break;
		case 'n':		/* switch start flag to 'N' */
		case 'N':
			nflg++;
			ch = 'N';
			if (lflg) {
				if (yflg||dflg||hflg)
					errflg++;
			} else if (yflg||dflg||hflg||pflg)
					errflg++;
			break;
		case 'd':		/* delete entry */
			dflg++;
			if (yflg||nflg||hflg||pflg||lflg)
				errflg++;
			break;
		case 'h':		/* set the ORACLE_HOME for SID */
			hflg++;
			if (yflg||nflg||dflg||pflg||lflg)
				errflg++;
			home = optarg;
			break;
		case 'p':
			pflg++;
			if (lflg) {
				if (nflg||dflg||hflg)
					errflg++;
			} else if (yflg||nflg||dflg||hflg) 
					errflg++;
			break;
		case '?':
			errflg++;
		}
	
	if (!(argc - optind == 1 || (argc - optind == 0 && lflg)))
		errflg++;

	if (lflg && argc - optind != 0)
		errflg++;

	sid = argv[optind];

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}

	if (stat(ORATAB, &statbuf) < 0) {
		fprintf(stderr, "cannot stat '%s'\n", ORATAB);
		exit(1);
	}

	if (!lflg) {
		if ((fp = fdopen(lock(ORATAB), "r")) == NULL) {
			fprintf(stderr, "%s: cannot open '%s' for locking\n", prog, ORATAB);
			exit(1);
		}
	} else {
		if ((fp = fopen(ORATAB, "r")) == NULL) {
			fprintf(stderr, "cannot open '%s' for reading\n", ORATAB);
			exit(1);
		}
	}
	if ((nflg||yflg||hflg||dflg) && !lflg)
		if ((fpt = fopen(tmpfile, "w")) == NULL) {
			fprintf(stderr,
				"%s: cannot open '%s' for writing\n", prog, tmpfile);
			exit(1);
		}

	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		if (buf[0] == '#' )
			goto print;

		if (sscanf((char *)buf, "%[^:]:%[^:]:%c", 
			oracle_sid, oracle_home, &start) != 3) {
			goto print;	/* invalid format */
		}

		if (lflg) {
			if ((yflg && start != 'Y') || *oracle_sid == '*')
				continue;
			if ((nflg && start != 'N') || *oracle_sid == '*')
				continue;
			if (pflg)
				printf("%s:%s:%c\n", oracle_sid, oracle_home, start);
			else
				printf("%s\n", oracle_sid);
			found++;
			continue;
		}

		if (strcmp(sid, (char*)oracle_sid) == 0) {
			if (nflg || hflg || yflg) {
				sprintf((char *)buf, "%s:%s:%c\n",
					(char *)oracle_sid,
					(hflg ? home : (char *)oracle_home),
					toupper(start) == 'S' ? 'S' : (nflg||yflg ? ch : start));
				found++;
			} else if (dflg) {
				found++;
				continue;
			} else { 		/* no flag - just check for entry */
				if (pflg)
					printf("%s:%s:%c\n", oracle_sid, oracle_home, start);
				exit(0);
			}
		}
	print:
		if ((nflg||yflg||hflg||dflg) && !lflg)
			if (fputs((char *)buf, fpt) == EOF) {
				perror("fputs");
				exit(errno);
			}
	}

	if (lflg) {
		if (!found)
			exit(1);
		exit(0);
	}

	if (dflg && !found)		/* entry not found to delete */
		exit(1);	

	if (hflg && !found++)	/* new entry */
		fprintf(fpt, "%s:%s:%c\n", sid, home, 'Y');

	if (!found)				/* entry did not exist or was not added */
		exit(1);		

	fclose(fp);
	fclose(fpt);

	/* copy back */
	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	if ((fp = fopen(ORATAB, "w")) == NULL) {
		fprintf(stderr, "%s: cannot open '%s' for writing\n", prog, ORATAB);
		exit(1);
	}
	if ((fpt = fopen(tmpfile, "r")) == NULL) {
		fprintf(stderr, "%s: cannot open '%s' for reading\n", prog, tmpfile);
		exit(1);
	}
	while (fgets((char *)buf, BUFSIZ, fpt) != NULL)
		if (fputs((char *)buf, fp) == EOF) {
			perror("fputs");
			exit(1);
		}

	fclose(fp);
	fclose(fpt);

	if (unlink(tmpfile) != 0) {
		perror("unlink");
		exit(1);
	}
	exit(0);
}


#include <unistd.h>
#include <fcntl.h>
/*
 * lock: do advisory locking on fd
 */
static int
#ifdef __STDC__
lock(char *path)
#else
lock(path)
 char *path;
#endif
{
	int fd, val;
	struct flock flk;
	char	buf[10];

	flk.l_type = F_WRLCK;
	flk.l_start = 0;
	flk.l_whence = SEEK_SET;
	flk.l_len = 0;

	if ((fd = open(path, O_RDWR)) < 0) {
		fprintf(stderr, "cannot open '%s' for read/write\n", path);
		exit(1);
	}
	
	/* attempt lock of entire file */

	if (fcntl(fd, F_SETLK, &flk) < 0) {
		if (errno == EACCES || errno == EAGAIN) {
			fprintf(stderr, "file '%s' being edited try later\n", path);
			exit(0);	/* gracefully exit */
		} else {
			fprintf(stderr, "error in attempting lock of '%s'\n", path);
			exit(1);
		}
	}

	/* set close-on-exec flag for fd */

	if ((val = fcntl(fd, F_GETFD, 0)) < 0) {
		perror("getfd");
		exit(1);
	}
	val |= FD_CLOEXEC;

	if (fcntl(fd, F_SETFD, val) < 0) {
		perror("setfd");
		exit(1);
	}

	/* leave file open until we terminate: lock will be held */

	return fd;
}
