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
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <regex.h>
#undef _LINUX_LIMITS_H
#ifdef linux
#include <linux/limits.h>
#endif

#define	USAGE "[-nigraf] [-m <mode>] [-o <opt>...] [[-s]-d <driver>[.drv]...] [-p <dir>] [-w <n>]\n\nwhere <mode> = apply|preinstall"

#define	MAXDRV		10
#define	RM			"rm -f $APPL_TOP/admin/$TWO_TASK/restart/*.rf9"
#ifdef DEBUG
#define	EXECCMD		"echo"
#define	SYSCMD		"echo " RM
#else
#define	EXECCMD		"adpatch"
#define	SYSCMD		RM
#endif
#define	ON			1
#define OFF			0

#define	MAXOPT		30
static char optionv[ARG_MAX] = "";

struct option {
	char	*name;
	int		flag;
};

/* only non-default values present */
static struct option optionlist[] = {
	"nocheckfile", 0,
	"nocompiledb", 0,
	"nocompilejsp", 0,
	"nocopyportion", 0,
	"nodatabaseportion", 0,
	"nogenerateportion", 0,
	"integrity", 0,
	"noautoconfig", 0,
	"noactiondetails", 0,
	"noparallel", 0,
	"noprereq", 0,
	"validate", 0,
	"hotpatch", 0,
	"maintenancemode", 0,
	"phtofile", 0,
	"-unknown-", 0
};

#ifdef __STDC
static int search_drv(char **, int, int);
char *drvname(const char *);
static int setopt(const char *, int);
static char *catopt(void);
static void printopt(void);
#else
static int search_drv();
char *drvname();
static int setopt();
static char *catopt();
static void printopt();
#endif

char *prog;

static char *appl_top, *sid, *def;

static char *nargv[1024];
static int nargc = 0;
static int showall = 0;

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	int		c, i;
	extern char	*optarg;
	extern int	optind;
	char	*drv[MAXDRV];
	pid_t	pid;
	int		pid_stat;
	int		ndrv = 0, num = 0;
	char	*dir, *mode;
	struct	stat statbuf;
	char	cwd[PATH_MAX], buf[PATH_MAX];
	int		nflg = 0, iflg = 0, gflg = 0, fflg = 0, dflg = 0, sflg = 0,
			pflg = 0, wflg = 0, aflg = 0, oflg = 0, mflg = 0, rflg = 0,
			errflg = 0;


	prog = basename(argv[0]);

	while ((c = getopt(argc, argv, "nigfard:sp:w:o:m:?")) != EOF)
		switch (c) {
		case 'n':		/* don't execute - just print */
			nflg++;
			break;
		case 'i':		/* interactive mode */
			iflg++;
			break;
		case 'a':		/* abandon previous session or ALREADY_APPLIED_PATCH */
			aflg++;
			break;
		case 'g':		/* generate default file */
			gflg++;
			break;
		case 'f':		/* wait on failed job or force overwrite of rf9s */
			fflg++;
			break;
		case 'r':		/* restart an existing session */
			rflg++;
			break;
		case 'd':		/* driver files */
			dflg++;
			if (ndrv < MAXDRV) {	/* ignore drivers after MAXDRV */
				char	*driver = drvname(optarg);

				if (strcmp(optarg, "all") == 0)
					showall++;
				
				for (i = 0 ; i < ndrv; i++) 
					if (strcmp(drv[i], driver) == 0) {
						fprintf(stderr, "%s: driver '%s' multiply specified\n",
							prog, optarg);
						exit(2);
					}
				drv[ndrv++] = driver;
			}
			break;
		case 's':		/* sort driver args */
			sflg++;
			break;
		case 'p':		/* patch top directory */
			pflg++;
			dir = optarg;
			break;
		case 'w':		/* number of parallel processes */
			wflg++;
			num = atoi(optarg);
			break;
		case 'o':		/* gather options */
			oflg++;
			setopt(optarg, ON);
			break;
		case 'm':		/* mode - only once */
			if (strcmp(optarg, "preinstall") == 0 )
				mode = "preinstall";
			else if (strcmp(optarg, "apply") == 0)
				mode = "apply";
			else
				errflg++;
			if (mflg)	
				errflg++;
			mflg++;
			break;
		case '?':
			errflg++;
		}

	if (sflg && !dflg)
		errflg++;

	if (argc - optind != 0)
		errflg++;

	if (showall) {
		if (ndrv > 1) {
			fprintf(stderr, "%s: keyword 'all' not allowed with multiple drivers\n",
				prog);
			exit(2);
		}
		ndrv = 0;
	}

	if (errflg) {
		fprintf(stderr, "usage: \n %s %s\n", prog, USAGE);
		fprintf(stderr, "\nwhere <opt> = \n ");
		printopt();
		fprintf(stderr, "\n");
		exit(2);
	}

	/* Patch Top */

	if (pflg)
		if (chdir(dir) != 0) {
			perror("chdir");
			exit(errno);
		}

	/* Defaults file */

	if ((appl_top = getenv("APPL_TOP")) == NULL) {
		fprintf(stderr, "%s: APPL_TOP not in environment\n", prog);
		exit(1);
	}

	if ((sid = getenv("TWO_TASK")) == NULL) {
		fprintf(stderr, "%s: TWO_TASK not in environment\n", prog);
		exit(1);
	}

	if ((def = (char *)malloc(strlen(appl_top) + strlen(sid) + 16)) == NULL) {
		perror("malloc");
		exit(errno);
	}
	sprintf(def, "%s/admin/%s/def.txt", appl_top, sid);

	if (!gflg) {
		if (getcwd(cwd, PATH_MAX + 1) == NULL) {
			perror("cwd");
			exit(errno);
		}

		/* adpatch will insist that the patch directory is writeable */

		if (access(cwd, W_OK|X_OK) != 0) {
			fprintf(stderr, "%s: directory '%s' not writeable/accessible\n",
				prog, cwd);
			exit(errno);
		}

		/* Test for backup directory */

		if (stat("backup", &statbuf) != 0) {
			if (errno == ENOENT) {
				if (mkdir("backup", S_IRWXU) != 0) {
					fprintf(stderr, "%s: cannot create directory '%s/backup'\n",
						prog, cwd);
					exit(errno);
				}
				if (chmod("backup", S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO) != 0) {
					fprintf(stderr, "%s: cannot chmod directory '%s/backup'\n",
						prog, cwd);
					exit(errno);
				}
			} else {
				fprintf(stderr, "%s: cannot stat '%s/%s'\n",
					prog, cwd, "backup");
				perror(NULL);
				exit(errno);
			}
		} else {
			if (access("backup", W_OK|X_OK) != 0) {
				if (statbuf.st_uid == geteuid()) {
					if (chmod("backup", S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO) != 0) {
						fprintf(stderr,
							"%s: cannot change mode for 'backup'\n",
							prog);
						exit(errno);
					}
				}
			}
		}

		/* Build list of drivers */

		ndrv = search_drv(drv, ndrv, sflg);
	}

	nargv[nargc++] = strdup("adbatch");

	if (mflg) {
		snprintf(buf, PATH_MAX, "%s=yes", mode);
		nargv[nargc++] = strdup(buf);
	}
	if (rflg) {
		snprintf(buf, PATH_MAX, "restart=yes");
		nargv[nargc++] = strdup(buf);
	}
	if (aflg) {
		snprintf(buf, PATH_MAX, "abandon=yes");
		nargv[nargc++] = strdup(buf);
	}
	if (fflg) {
		snprintf(buf, PATH_MAX, "wait_on_failed_job=yes");
		nargv[nargc++] = strdup(buf);
	}

	snprintf(buf, PATH_MAX, "defaultsfile=%s", def);
	nargv[nargc++] = strdup(buf);

	if (!gflg) {
		snprintf(buf, PATH_MAX, "logfile=%s.log", basename(cwd));
		nargv[nargc++] = strdup(buf);

		snprintf(buf, PATH_MAX, "patchtop=%s", cwd);
		nargv[nargc++] = strdup(buf);

		if (ndrv) {
			snprintf(buf, PATH_MAX, "driver=");
			for (i = 0; i < ndrv; i++)
				snprintf(buf, PATH_MAX, "%s%s%c", buf, drv[i], i + 1 < ndrv ? ',' : '\0');
			nargv[nargc++] = strdup(buf);
		}

		snprintf(buf, PATH_MAX, "workers=%d",
#ifdef _HPUX_SOURCE
			wflg ? num : mpctl(MPC_GETNUMSPUS_SYS, 0, 0) + 2);
#else
			wflg ? num : sysconf(_SC_NPROCESSORS_ONLN) + 2);
#endif
		nargv[nargc++] = strdup(buf);

		snprintf(buf, PATH_MAX, "interactive=%s", iflg ? "yes" : "no");
		nargv[nargc++] = strdup(buf);

		nargv[nargc++] = catopt();
	} else {
		/* later versions of adpatch require maintenance mode to be enabled, 
		 * however, only the very latest versions support the maintenancemode
		 * option: as a common denominator we therefore use "hotpatch"
		 */
		setopt("hotpatch", ON);
		nargv[nargc++] = catopt();
	}

	nargv[nargc] = NULL;

	if (gflg && !nflg) {
		char	line[10];

		fflush(stdout);
		setbuf(stdout, NULL); /* in case I am in a pipeline or co-process */

		if (!fflg) {
			printf("Restart information will be removed. Continue? (yes/no): ");

			if (!isatty(0))
				printf("\n");

			scanf("%s", line);
			if (strcmp(line, "yes") != 0)
				exit(3);
		}

		/* remove restart files */
		system(SYSCMD);

		/* remove defaults file - otherwise it will be invoked! */
		if (access(def, R_OK) == 0) 
			if (unlink(def) != 0) {
				fprintf(stderr, "%s: unlink of '%s' failed\n", prog, def);
				exit(errno);
			}
	}

	if (nflg) {
		printf("adpatch ");
		for (i = 1; i < nargc; i++)
			printf("%s ", nargv[i]);
			
		printf("\n");
		exit(0);
	}
	if (fflg && !(nflg||gflg))
		system(SYSCMD);

	if (!gflg) {
		execvp(EXECCMD, nargv);
		perror("exec");
		exit(errno);
	}

	switch (pid = fork()) {
	case 0:	/* child */
		execvp(EXECCMD, nargv);
		perror("exec");
		exit(errno);
	case -1:	/*error */
		perror("fork");
		exit(errno);
	}
	waitpid(pid, &pid_stat, 0);

	if (gflg) {
		FILE *fp;
		system(SYSCMD);

		if ((fp = fopen(def, "a")) == NULL) {
			fprintf(stderr, "%s: failed to open %s\n", prog, def);
			exit(1);
		} 
		fputs("## Start of Defaults Record\n", fp);
		fputs("  %%START_OF_TOKEN%%\n", fp);
		fputs("\tACTION_TURNED_OFF\n", fp);
		fputs("  %%END_OF_TOKEN%%\n", fp);
		fputs("\n", fp);

		fputs("  %%START_OF_VALUE%%\n", fp);
		fputs("\tYes\n", fp);
		fputs("  %%END_OF_VALUE%%\n", fp);
		fputs("## End of Defaults Record\n", fp);
		fputs("\n", fp);

		if (aflg) {
			fputs("## Start of Defaults Record\n", fp);
			fputs("  %%START_OF_TOKEN%%\n", fp);
			fputs("\tALREADY_APPLIED_PATCH\n", fp);
			fputs("  %%END_OF_TOKEN%%\n", fp);
			fputs("\n", fp);

			fputs("  %%START_OF_VALUE%%\n", fp);
			fputs("\tYes\n", fp);
			fputs("  %%END_OF_VALUE%%\n", fp);
			fputs("## End of Defaults Record\n", fp);
			fputs("\n", fp);
		}

		fclose(fp);
	}

	exit(0);
}

#ifdef GLOB
#include <glob.h>
#else
#include <wordexp.h>
#endif

static int
#ifdef __STDC__
compare(char **pi,  char **pj)
#else
compare(pi, pj)
 char **pi;
 char **pj;
#endif
{
	return strcmp(*pi, *pj);
}

/*
 * search: return number of drivers found
 */
static int
#ifdef __STDC__
search_drv(char **drv, int ndrv, int sort)
#else
search_drv(drv, ndrv, sort)
 char **drv;
 int ndrv;
 int sort;
#endif
{
#ifdef GLOB
	char *wrds[] = {
        "u[0-9]*.drv",
        "[cdg]merged.drv",
        "[cdg][0-9]*.drv",
        NULL
	};
	glob_t		globbuf;
	char		**p = wrds;
	int			g = 0;				/* number of times glob() has been called */
	int			n;
#else
	char		*wrds = "[cdg]merged.drv u*.drv";
	char		*allwrds = "merged.drv u*.drv c*.drv d*.drv g*.drv";
	wordexp_t	wrdexp;
	int			matches = 0;
	static int	n = 0;
#endif
	int			i;


#ifdef GLOB
	while (*p) {
		if ((n = glob(*(p++), g++ > 0 ? GLOB_APPEND : 0, NULL, &globbuf)) != 0) {
			if (n != GLOB_NOMATCH) {
				perror("glob");
				exit(errno);
			}
		}
	}
#else
	if (showall)
		wrds = allwrds;
	if (wordexp(wrds, &wrdexp, WRDE_NOCMD|(n++ > 0 ? WRDE_REUSE : 0)) != 0) {
		perror("wordexp");
		exit(errno);
	}
#endif

#ifdef GLOB
	for (i = 0; i < (ndrv ? ndrv : globbuf.gl_pathc); i++) {
#else
	for (i = 0; i < (ndrv ? ndrv : wrdexp.we_wordc); i++) {
#endif
		if (ndrv) {
			if (access(drv[i], R_OK) != 0) {
				fprintf(stderr, "%s: cannot read '%s'\n", prog, drv[i]);
				exit(1);
			}
		} else {
#ifdef GLOB
			*(drv++) = strdup(globbuf.gl_pathv[i]);
#else
			if (access(wrdexp.we_wordv[i], R_OK) == 0) {
				*(drv++) = strdup(wrdexp.we_wordv[i]);
				matches++;
			}
#endif
		}
	}

#ifdef GLOB
	i = globbuf.gl_pathc;	/* save path count before globfree() */
	globfree(&globbuf);
#endif

	if (!ndrv)
#ifdef GLOB
		return i;
#else
		return matches;	/*return wrdexp.we_wordc;*/
#endif

	if (sort)
		qsort((void *)drv, ndrv, sizeof(char *),
#ifdef __STDC__
			(int (*)(const void *, const void *))compare);
#else
			(int (*)())compare);
#endif
	return ndrv;
}

/*
 * match: match string s against regular expression pattern 
 */
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
/*
 * drvname: condition driver name - must end ".drv"
 */
char *
#ifdef __STDC__
drvname(const char *s)
#else
drvname(s)
 char *s;
#endif
{
	char	*t;

	if (match(s, ".*\\.drv$"))
		return (strdup(s));

	if ((t = (char *)malloc(strlen(s) + 5)) == NULL) {
		perror("malloc");
		exit(errno);
	}
	strcpy(t, s);
	strcat(t, ".drv");

	return t;
}

static int
#ifdef __STDC
setopt(const char *s, int n)
#else
setopt(s, n)
 char *s;
 int n;
#endif
{
	int		i = 0;
	while (strcmp(optionlist[i].name, "-unknown-")) {
		if (strcmp(optionlist[i].name, s) == 0) {
			optionlist[i].flag = n;
			return 1;
		}
		i++;
	}
	return 0;
}

static char *
#ifdef __STDC__
catopt(void)
#else
catopt()
#endif
{
	int		i = 0;
	while (strcmp(optionlist[i].name, "-unknown-")) {
		if (optionlist[i].flag) {
			if (strlen(optionv)) 
				sprintf(optionv, "%s,%s", optionv, optionlist[i].name);
			else
				sprintf(optionv, "options=%s", optionlist[i].name);
		}
		i++;
	}
	if (strlen(optionv))
		return optionv;
	return NULL;
}

static void
#ifdef __STDC__
printopt(void)
#else
printopt()
#endif
{
	int		i = 0;
	while (strcmp(optionlist[i].name, "-unknown-")) {
		fprintf(stderr, "%s%s", i ? ",\n " : "", optionlist[i].name);
		i++;
	}
}
