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
 * dbinfo - get information on database - how to start/stop; version etc
 */

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/param.h>
#define _DBINFO
#include "dbinfo.h" 

#ifdef __STDC__
extern char *argcat(const char *, const char *);
extern struct cmdstr *setcmd(struct cmdstr *, const char *);
extern char *getcmd(const char *);
extern char *getscript(const char *, const char *);
#else
extern char *argcat();
extern struct cmdstr *setcmd();
extern char *getcmd();
extern char *getscript();
#endif

										/* version is limited to R.L.B.S */
#define PAT	".*[RV]e[lr][es][ai][os][en] \\([^ \\.]\\{1,\\}\\.[^ \\.]\\{1,\\}\\.[^ \\.]\\{1,\\}\\.[^ \\.]\\{1,\\}\\)"

#ifdef __STDC__
static char	*getvers(struct cmdstr *);
static char	*ld_library_path;
static char *regcpy(int, const char *);
#else
static char	*getvers();
static char	*ld_library_path;
static char *regcpy();
#endif


struct cmdstr *
#ifdef __STDC__
dbinfo(struct cmdstr *pcmdstr, const char *oracle_home)
#else
dbinfo(pcmdstr, oracle_home)
 struct cmdstr *pcmdstr;
 char *oracle_home;
#endif
{
	struct	stat statbuf;
	char	*sqldba, *script;
	struct	passwd *pw;
	struct cmdstr *p = pcmdstr;
	char	*oh;

	/* ORACLE_SID is not necessary to determine the version but will
	   be required to actually run a command. */

	p->oracle_sid = getenv("ORACLE_SID");

	/* Test ORACLE_HOME */

	if (oracle_home) {
		if (p->oracle_home)
			free(p->oracle_home);
		p->oracle_home = strdup(oracle_home);
	} else {
		if ((p->oracle_home = getenv("ORACLE_HOME")) == NULL) {
			fprintf(stderr, "ORACLE_HOME must be in environment\n");
			return NULL;
		} else if (strlen(p->oracle_home) == 0) {
				fprintf(stderr, "ORACLE_HOME must be non-null\n");
				return NULL;
			}
	}

	if (stat(p->oracle_home, &statbuf) < 0) {
		fprintf(stderr, "cannot stat %s\n", p->oracle_home);
		return NULL;
	} else {
		char	*t;
		if ((t = (char *)malloc(
				strlen(p->oracle_home) +
				strlen("/bin/oracle") + 1)) == NULL) {
			perror("malloc");
			exit(errno);
		}
		if (sprintf(t, "%s/bin/oracle", p->oracle_home) < 0) {
			perror("sprintf");
			exit(errno);
		}
		if (stat(t, &statbuf) < 0) {
			if (errno == ENOENT) {
				fprintf(stderr, "Invalid ORACLE_HOME '%s'\n", p->oracle_home);
				return NULL;
			} else {
				perror("stat");
				return NULL;
			}
		}
		if (!S_ISREG(statbuf.st_mode)||!(statbuf.st_mode & S_ISUID)) {
			fprintf(stderr, "Invalid ORACLE_HOME '%s' - no setuid oracle\n",
				 p->oracle_home);
			exit(1);
		}
		p->uid = statbuf.st_uid;
		p->gid = statbuf.st_gid;

		free(t);
	}

	if ((pw = getpwuid(p->uid)) == NULL) {
		perror("getpwuid");
		return NULL;
	}
	p->name = strdup(pw->pw_name);

	/* Put ORACLE_HOME/lib into LD_LIBRARY_PATH (even though this should
	 * no longer be used some older programs may have been compiled with
	 * dyanamic paths.)
	 */
	if ((ld_library_path = (char *)malloc(strlen("LD_LIBRARY_PATH") +
			strlen(p->oracle_home) +
			strlen("/lib") + 2)) == NULL) {
		perror("malloc");
		return NULL;
	}
	sprintf(ld_library_path, "LD_LIBRARY_PATH=%s/lib", p->oracle_home);
	putenv(ld_library_path);

	/* Need ORACLE_HOME for messages etc */

	if ((oh = (char *) malloc(strlen("ORACLE_HOME= ")
		+ strlen(p->oracle_home))) == NULL) {
		fprintf(stderr, "dbinfo(): malloc\n");
		return NULL;
	}
	sprintf(oh, "ORACLE_HOME=%s", p->oracle_home);
	putenv(oh);
	
	/* Which programs do I have? */
	
	p->connect = USER(INTERNAL);

	/* Use program that can print version */

	//if ((p = setcmd(pcmdstr, "sqldba")) != NULL)  			/* V5, V6 */
	//	p->args[0] = TEST(T_SQLDBA);
	//else if ((p = setcmd(pcmdstr, "svrmgrl")) != NULL)		/* V7, V8 */
	//	p->args[0] = TEST(T_SQLDBA);
	//else
	if ((p = setcmd(pcmdstr, "sqlplus")) != NULL)  {	/* V8.1, V9 */
		p->args[0] = TEST(T_SQLPLUS);
		p->connect = USER(SYSDBA);
	} else {
		fprintf(stderr, "Cannot find a program \n");
		return NULL;
	}

	/* Figure out version */

	if ((p->version = getvers(p)) == NULL) {
		fprintf(stderr, "Cannot determine version\n");
		return NULL;
	}

	return p;
}

#include <regex.h>
static regex_t		re;							/* compile struct */
static regmatch_t	pm[2];						/* array of match sructs */

/*
 * getvers: extract the version string from that returned by the command
 */
static char *
#ifdef __STDC__
getvers(struct cmdstr *p)
#else
getvers(p)
 struct cmdstr *p;
#endif
{
	char	*pat = PAT;
	char	buf[BUFSIZ];
	FILE	*fp;
	int		n;
	char	*cmdstr, *v = NULL;

	cmdstr = argcat(p->path, p->args[0]);

	if ((fp = popen(cmdstr, "r")) == NULL) {
		perror("popen");
		exit(errno);
	}

	if ((n = regcomp(&re, pat, 0)) != 0)
		goto error;

	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		if ((n = regexec(&re, (char *)buf, re.re_nsub + 1, pm, 0)) == 0) {
			if ((v = regcpy(1, (char *)buf)) == NULL) 
				return NULL;
		}
	}
	free(cmdstr);
	return v;

error:
	regerror(n, &re, buf, sizeof buf);
    regfree(&re);
	free(cmdstr);
    return NULL;
}

static char *
#ifdef __STDC__
regcpy(int n, const char *s)
#else
regcpy(n, s)
 int n;
 char *s;
#endif
{
	char *t;
	
	if ((t = (char *) malloc(pm[n].rm_eo - pm[n].rm_so + 1)) == NULL) {
		fprintf(stderr, "malloc error (%d)\n", errno);
		return NULL;
	}

	memcpy(t, s + pm[n].rm_so, pm[n].rm_eo - pm[n].rm_so);
	memset(t + pm[n].rm_eo - pm[n].rm_so, '\0', 1); /*strcat(t, ""); */

	return t;
}
