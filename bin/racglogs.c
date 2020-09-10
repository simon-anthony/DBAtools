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
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#define USAGE "-d database -i instance [-n <n>] start|stop"
#define PAT "Attempting to %s `ora\\.%s\\.%s\\.inst` on member `%s`"

#ifdef __STDC__
int match(const char *, char *);
char *getoralog(char *, char *, char *, char *);
int getlogevent(char *, time_t, int);
int getlogevents(char *, time_t, int);
#else
int match();
char * getoralog();
int getlogevent();
int getlogevents();
#endif

char *prog;
char node[MAXHOSTNAMELEN];

int
main(int argc, char *argv[])
{
	FILE	*fp;
	char	buf[BUFSIZ], startbuf[BUFSIZ];
	char	lv[BUFSIZ], rv[BUFSIZ];
	int		n;
	struct	tm t;
	time_t	time = 0;
	char	*crsdlog, *racglog, *racglogdir, *db, *inst;
	int		c, i;
	char	*resource;	/* CRS resource name */
	char	*name, *event;
	extern char	*optarg;
	extern int	optind;
	char 	*pat, *imon;
	int 	maxlines = 0;	/* all lines */

	int		dflg = 0, iflg = 0, nflg = 0, errflg = 0;

	prog = strdup(basename(argv[0]));

	while ((c = getopt(argc, argv, "d:i:n:")) != EOF)
		switch (c) {
		case 'd':		/* database */
			db = optarg;
			dflg++;
			break;
		case 'i':		/* instance */
			inst = optarg;
			iflg++;
			break;
		case 'n':		/* number of lines */
			if ((maxlines = atoi(optarg)) == 0) {
				perror("atoi");
				errflg++;
			}
			nflg++;
			break;
		case '?':
			errflg++;
		}

	if (argc - optind != 1)
		errflg++;

	if (!(strcmp(argv[optind], "stop") == 0 || strcmp(argv[optind], "start") ==
	0))
		errflg++;
	event = argv[optind];

	if (!dflg || !iflg)
		errflg++;

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}

	if ((name = getenv("ORA_CRS_HOME")) == NULL) {
		fprintf(stderr, "%s: ORA_CRS_HOME not set\n", prog);
		exit(1);
	}
	if (gethostname((char *)node, MAXHOSTNAMELEN) < 0) {
		perror("gethostname");
		exit(errno);
	}
	if ((crsdlog = getoralog(name, (char *)node, "crsd", "crsd")) == NULL) {
		fprintf(stderr, "%s: cannot determine crsd log\n", prog);
		exit(1);
	}

	if ((pat = (char *)malloc(
			strlen(PAT) + strlen(event) + strlen(db) + strlen(inst) +
			strlen(node) + 1)) == NULL) {
		perror("malloc");
		exit(errno);
	}
	sprintf(pat, PAT, event, db, inst, node);

	printf("crsd log is %s\n", crsdlog);

	if ((fp = fopen(crsdlog, "r")) == NULL) {
		fprintf(stderr, "%s\n", crsdlog);
		perror("fopen");
		exit(1);
	}
	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		if (buf[0] == '#'||buf[0] == '\n' )
			continue;

		n = sscanf((char *)buf, "%[^.]%*[.0-9: ]%[^\n]", lv, rv);
		if (n != 2)
			continue;

		if (!match(rv, pat))
			continue;

		if (strptime(lv, "%Y-%m-%d %T", &t) == NULL) {
			fprintf(stderr, "strptime: cannot convert time %s\n", lv);
			continue;
		}
		if ((time = mktime(&t)) < 0) {
			fprintf(stderr, "time: cannot convert time\n");
			continue;
		}
		strftime(lv, BUFSIZ, "%a %b %e %T %Y", &t);
		sprintf(startbuf, "[  ]%s: %s\n", lv, rv);
	}

	if (time) {
		printf("%s", (char *)startbuf);
		printf("Last attempted %s: %s", event, ctime(&time));
	} else {
		fprintf(stderr, "%s: cannot find attempted start for %s\n", prog, inst);
		exit(1);
	}
		
	if ((name = getenv("ORACLE_HOME")) == NULL) {
		fprintf(stderr, "%s: ORACLE_HOME not set\n", prog);
		exit(1);
	}
	if ((imon = (char *)malloc(strlen("imon_") + strlen(db) + 1)) == NULL) {
		perror("malloc");
		exit(errno);
	}
	sprintf(imon, "imon_%s", db);
	if ((racglog = getoralog(name, (char *)node, "racg", imon)) == NULL) {
		fprintf(stderr, "%s: cannot determine RAC instance monitor log\n",
			prog);
		exit(1);
	}
	racglogdir = dirname(strdup(racglog));
	getlogevents(racglogdir, time, maxlines);

	exit(0);
}

char *
getcrsdlog(char *crs, char *host)
{
	char *path;
	char *fmt = "%s/log/%s/crsd/crsd.log";

	if ((path = (char *)malloc(
			strlen(crs) + strlen(host) + strlen(fmt))) == NULL) {
		perror("malloc");
		return NULL;
	}

	sprintf(path, fmt, crs, host);

	if (access(path, R_OK) != 0) {
		fprintf(stderr, "%s: cannot access %s\n", prog, path);
		return NULL;
	}

	return path;
}

char *
getoralog(char *oh, char *host, char *dir, char *log)
{
	char *path;
	char *fmt = "%s/log/%s/%s/%s.log";

	if ((path = (char *)malloc(
			strlen(oh) + strlen(host) + strlen(dir) +
			strlen(log) + strlen(fmt))) == NULL) {
		perror("malloc");
		return NULL;
	}

	sprintf(path, fmt, oh, host, dir, log);

	if (access(path, R_OK) != 0) {
		fprintf(stderr, "%s: cannot access %s\n", prog, path);
		return NULL;
	}

	return path;
}

int
getlogevent(char *path, time_t reftime, int max)
{
	FILE	*fp;
	char	buf[BUFSIZ];
	char	lv[BUFSIZ], rv[BUFSIZ];
	int		n;
	struct	tm t;
	time_t	time;
	int		i = 0;
	
	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "%s\n", path);
		perror("fopen");
		return 0;
	}
	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		if (buf[0] == '#'||buf[0] == '\n' )
			continue;

		n = sscanf((char *)buf, "%[^.]%*[.0-9: ]%[^\n]", lv, rv);
		if (n != 2)
			continue;

		if (strptime(lv, "%Y-%m-%d %T", &t) == NULL) 
			continue;	/* assume not timed event */

		if ((time = mktime(&t)) < 0) {
			fprintf(stderr, "mktime: cannot convert time\n");
			continue;
		}
		if (time >= reftime) {
			strftime(lv, BUFSIZ, "%a %b %e %T %Y", &t);
			printf("%s: %s\n", lv, rv);
		}
		if (max)
			if (i++ == max)
				return(0);
	}
	return(0);
}

#include <glob.h>
int
getlogevents(char *path, time_t reftime, int max)
{
	FILE	*fp;
	char	buf[BUFSIZ];
	char	lv[BUFSIZ], rv[BUFSIZ];
	int		n;
	struct	tm t, *tp;
	time_t	time;
	int		line;
	glob_t	globbuf;
	struct	stat statbuf;
	
	if (chdir(path) != 0) {
		fprintf(stderr, "%s: cannot cd to%s\n", prog, path);
		return 0;
	}
	printf("Checking logs in %s\n", path);
	if (n = glob("*.log", 0, NULL, &globbuf))
		if (n != GLOB_NOMATCH) {
			perror("glob");
			return 0;
		}
	if (globbuf.gl_pathc > 0) {
		int i;
		for (i = 0; i < globbuf.gl_pathc; i++) {
			if (stat(globbuf.gl_pathv[i], &statbuf) != 0) {
				fprintf(stderr, "cannot stat %s\n", globbuf.gl_pathv[i]);
				continue;
			}
			if (statbuf.st_mtime < reftime)
				continue;	/* file has not been modified since */

			tp = localtime(&statbuf.st_mtime);
			strftime(lv, BUFSIZ, "%a %b %e %T %Y", tp);
			printf("%s %s\n", (char *)lv, globbuf.gl_pathv[i]);

			if ((fp = fopen(globbuf.gl_pathv[i], "r")) == NULL) {
				fprintf(stderr, "cannot open: %s\n", globbuf.gl_pathv[i]);
				perror("fopen");
				continue;
			}
			line = 0;

			while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
				if (buf[0] == '#'||buf[0] == '\n' )
					continue;

				n = sscanf((char *)buf, "%[^.]%*[.0-9: ]%[^\n]", lv, rv);
				if (n != 2)
					continue;

				if (strptime(lv, "%Y-%m-%d %T", &t) == NULL)
					continue; /* assume not timed event */
				
				if ((time = mktime(&t)) < 0) {
					fprintf(stderr, "mktime: cannot convert time\n");
					continue;
				}
				if (time >= reftime) {
					strftime(lv, BUFSIZ, "%a %b %e %T %Y", &t);
					printf("[%2d]%s: %s\n", line, lv, rv);
				} else
					continue;
				if (max)
					if (line == max)
						break;
				line++;
			}
			fclose(fp);
		}
	}
	return(1);
}

/*
 * match: match string s against regular expression pattern 
 */
#include <regex.h>
int
#ifdef __STDC__
match(const char *s, char *pattern)
#else
match(s, pattern)
 char *s;
 char *pattern;
#endif
{
	int		status;
	regex_t	re;

	if (regcomp(&re, pattern, REG_NOSUB) != 0)
		return(0);

	status = regexec(&re, s, (size_t)0, NULL, 0);
	regfree(&re);

	if (status != 0)
		return(0);

	return(1);
}
