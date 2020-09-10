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
 * dbacmd - figure out how to start/stop database
 */

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/wait.h>
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
#include "dbopts.h"
#include <dbinfo.h>

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

int debug = 0;

#define USAGE "[-nd] {startup [opts...]|shutdown [opts...]}"
#define USAGE_DBVERS "[-d][-1|-2|-3|-4]"
#define USAGE_DBID "[-d]"

#define MSG_SHUTDOWN "ORACLE instance shut down."
#define MSG_STARTED	 "ORACLE instance started."
#define MSG_MOUNTED  "Database mounted."
#define MSG_OPENED   "Database opened."

char *prog;

#ifdef __STDC__
//static char	*getvers(struct cmdstr *);
static int runcmd(struct cmdstr *, const char *, const char*);
#else
//static char *getvers();
static int runcmd();
#endif

static int	nflg = 0, dflg = 0, n1flg = 0, n2flg = 0, n3flg = 0, n4flg = 0,
			errflg = 0;

static struct cmdstr command;

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
	char	*action, *option = NULL;
	struct	cmdstr *p;
	struct	stat statbuf;
	char	*sqldba, *script;
	int		c;
	int		n = 4;	/* default depth of version information for dbvers */

	prog = basename(argv[0]);

	while ((c = getopt(argc, argv, "nd1234")) != -1)
		switch (c) {
		case 'n':		/* just echo actions */
			nflg++;
			break;
		case 'd':		/* debug */
			debug++;
			break;
		case '1':		/* version depth */
			if (n2flg||n3flg||n4flg)
				errflg++;
			n = 1;
			break;
		case '2':
			if (n1flg||n3flg||n4flg)
				errflg++;
			n = 2;
			break;
		case '3':
			if (n1flg||n2flg||n4flg)
				errflg++;
			n = 3;
			break;
		case '4':
			if (n1flg||n2flg||n3flg)
				errflg++;
			n = 4;
			break;
		case '?':
			errflg++;
		}
#ifdef _AIX
	unsetenv("LIBPATH");
#endif
#ifdef _HPUX_SOURCE
	unsetenv("SHLIB_PATH");
#endif
	unsetenv("LD_LIBRARY_PATH");
	
	if (strcmp(prog, "dbvers")&& strcmp(prog, "dbid")) {	/* dbacmd */
		if (!(argc - optind))
			errflg++;
		else {	
			action = argv[optind];
			if (!strcmp(action, "startup")) 		/* startup or shutdown */
				setprog("startup");
			else if (!strcmp(action, "shutdown")) {
				setprog("shutdown");
			} else {
				errflg++;
				goto err;
			}

			if (argc - optind >= 2) {
				optind++;
				while (optind < argc && *argv[optind] != '-') {
					if (chkopt(argv[optind], SET)) {
						optind++;
					} else 
						break;
				}
			}

			if (!valopt())
				errflg++;
			else
				option = stropt();
		}
	} else
		if (argc - optind != 0)
			errflg++;

err:
	if (errflg) {
		if (strcmp(prog, "dbvers") && strcmp(prog, "dbid")) {
			if (argc - optind) {
				if (!strcmp(action, "shutdown"))
					fprintf(stderr, "usage: %s %s\n", prog, USAGE_DBSHUT);
				else if (!strcmp(action, "startup"))
					fprintf(stderr, "usage: %s %s\n", prog, USAGE_DBSTART);
				else
					fprintf(stderr, "usage: %s %s\n", prog, USAGE);
					
			} else
				fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		} else if (strcmp(prog, "dbid"))
			fprintf(stderr, "usage: %s %s\n", prog, USAGE_DBVERS);
		else 
			fprintf(stderr, "usage: %s %s\n", prog, USAGE_DBID);
		exit(2);
	}


	/* Get DB info: ORACLE_HOME must be set at this point */

	if ((p = dbinfo(&command, NULL)) == NULL) {
		fprintf(stderr, "%s: cannot determine information\n", prog);
		exit(1);
	}

	if (!strcmp(prog, "dbid")) {
		printf("%s\n", p->name);
		exit(0);
	}

	if (!strcmp(prog, "dbvers")) {
		if (n < 4) {
			char *pp = p->version;
			int i;
			for (i = 0; i < n; i++) {
				while (*pp != '.')
					pp++;
				if (i+1 != n)
					pp++;
			}
			*pp = '\0';
			printf("%s\n", p->version);
		} else
			printf("%s\n", p->version);
		exit(0);
	}

	/* ORACLE_SID required beyond this point */

	if (p->oracle_sid == NULL) {
		fprintf(stderr, "%s: ORACLE_SID not set\n", prog);
		exit(1);
	}

#ifdef APPS
	/* Search for a script to execute  - a new variable is used here because
	 * if nothing is found, the sqldba command will default to that found
	 * previously
     */
	if ((script = getscript(p->oracle_home, p->oracle_sid)) != NULL) {
		if ((p = setcmd(&command, script)) != NULL)
			exit(runcmd(&command, strcmp(action, "startup") ? "stop" : "start", option));
		exit(1);
	}
#endif

	/* Find command to use to shutdown/startup based on compatibility */

	if ((sqldba = getcmd(p->version)) == NULL) {
		if (debug) fprintf(stderr, "%s: no match\n", prog);
	} else if ((p = setcmd(&command, sqldba)) == NULL) {
		p = &command;
		if (debug) {
			fprintf(stderr, "%s: invalid command: %s, ", prog, sqldba);
			fprintf(stderr, "%s: defaulting to: %s\n", prog, basename(p->path));
		}
	}

	/* Set command line arguments for program */

	if (!fnmatch("*/sqlplus", p->path, 0)) {
		p->args[0] = CMDLINE(R_NOLOG);
		if (!fnmatch("8.1.*", p->version, 0))
			p->connect = USER(SYSDBA);
	} else if (!fnmatch("*/sqldba", p->path, 0)) {
		p->args[0] = CMDLINE(R_COMMAND); 
		p->args[1] = action;
		if (option)
			p->args[2] = option;
	} else if (!fnmatch("*/svrmgrl", p->path, 0)) {
		p->args[0] = "";
	}

	/* Run the command */

	exit(runcmd(p, action, option));
}

static int
#ifdef __STDC__
runcmd(struct cmdstr *p, const char *act, const char *opt)
#else
runcmd(p, act, opt)
 struct cmdstr *p;
 char *act;
 char *opt;
#endif
{
	FILE	*fp;
	char	*command, *action;
	struct passwd *pw;
	int		status = 0;
	int		success = 0;
#ifndef USE_POPEN
	char	ibuf[BUFSIZ];
	char	buf[BUFSIZ];
	int		opfd[2];	/* will write on this one */
	int		ipfd[2];	/* will read on this one */
#endif

	if ((pw = getpwuid(p->uid)) == NULL) {
		perror("getpwuid");
		exit(errno);
	}
	if (opt)									/* action (+option)*/
		action = argcat(act, opt);
	else
		action = (char *)act;

	if (!nflg) {
		if (getuid() == 0)						/* real user is root */
			if (p->uid != (uid_t)0) {
				setgid(p->gid);					/* change to dba group */
				setuid(p->uid);					/* change to oracle owner */
				if (chdir(pw->pw_dir) < 0) {	/* cd to oracle owner's dir */
					perror("chdir");
					exit(errno);
				}
			}
	} else {
		if (getuid() == 0)						/* real user is root */
			if (p->uid != (uid_t)0) {
				printf("su %s -c \"", pw->pw_name);
				fflush(stdout);
			}
	}	
	if (fnmatch("*/sqldba", p->path, 0)&&fnmatch("*/appsutil/*", p->path, 0)) {
		/* sqlplus & svrmgrl are piped their actions */
		if (nflg)									/*command */
			command = strdup("/bin/cat");
		else
#ifdef USE_POPEN
			command = argcat(p->path, p->args[0]);

		if ((fp = popen(command, "w")) == NULL) {
			perror("popen");
			exit(errno);
		}
		if (nflg) 
			fprintf(fp, "%s %s <<-!\n", p->path, p->args[0]);

		if (strncmp(action, "shutdown", 8) == 0) {
			if (fprintf(fp, "whenever sqlerror exit 1;\n") == EOF) 
				goto piperr;
		} else {
			if (fprintf(fp, "whenever sqlerror exit 9;\n") == EOF) 
				goto piperr;
		}

		if (fprintf(fp, "connect %s;\n", p->connect) == EOF) 
			goto piperr;

		
		if (fprintf(fp, "%s\n", action) == EOF)
			goto piperr;

		if (fprintf(fp, "exit\n") == EOF)
			goto piperr;

		if (nflg)
			if (getuid() == 0 && p->uid != (uid_t)0) 
				fprintf(fp, "!\"\n");
			else
				fprintf(fp, "!\n");
		if (pclose(fp) == -1) {
			perror("pclose");
			exit(errno);
		}
#else
			command = p->path;

		if (pipe(opfd) == -1) {		/* set up pipelines */
			perror("opipe");
			exit(1);
		}
		if (pipe(ipfd) == -1) {
			perror("ipipe");
			exit(1);
		}

		switch (fork()) {
		case -1:
			perror("fork");
			exit(1);
		case 0:	/* child */
			if (close(0) == -1) {		/* make fd 0 (stdin) available */
				perror("close0c");
				exit(1);
			}
			if (dup(opfd[0]) != 0) {	/* make stdin output pipe */
				perror("odup0c");
				exit(1);
			}
			if (close(opfd[0]) == -1 || close(opfd[1]) == -1 ) {	/* don't need */
				perror("close2c");
				exit(1);
			}
			if (close(1) == -1) {		/* make fd 1 (stdout) available */
				perror("close1c");
				exit(1);
			}
			if (dup(ipfd[1]) != 1) {	/* make stdout input pipe */
				perror("idup1c");
				exit(1);
			}
			if (close(ipfd[0]) == -1 || close(ipfd[1]) == -1 ) {	/* don't need */
				perror("close2c");
				exit(1);
			}
			execl(command, command, "-s", (nflg ? (char *)NULL : p->args[0]), (char *)NULL);
			perror("exec");
			exit(1);
		}
		/* parent */
		if (nflg) {
			snprintf((char *)ibuf, BUFSIZ, "%s %s <<-!\n", p->path, p->args[0]);
			write(opfd[1], ibuf, strlen(ibuf));
		}

		if (strncmp(action, "shutdown", 8) == 0) {
			snprintf((char *)ibuf, BUFSIZ, "whenever sqlerror exit 1;\n");
			write(opfd[1], ibuf, strlen(ibuf));
		} else {
			snprintf((char *)ibuf, BUFSIZ, "whenever sqlerror exit 9;\n");
			write(opfd[1], ibuf, strlen(ibuf));
		}

		snprintf((char *)ibuf, BUFSIZ, "connect %s;\n", p->connect);
		write(opfd[1], ibuf, strlen(ibuf));

		snprintf((char *)ibuf, BUFSIZ, "%s\n", action);
		write(opfd[1], ibuf, strlen(ibuf));

		snprintf((char *)ibuf, BUFSIZ, "exit\n");
		write(opfd[1], ibuf, strlen(ibuf));

		if (nflg)
			if (getuid() == 0 && p->uid != (uid_t)0) {
			snprintf((char *)ibuf, BUFSIZ, "!\"\n");
			write(opfd[1], ibuf, strlen(ibuf));
			} else {
				snprintf((char *)ibuf, BUFSIZ, "!\n");
				write(opfd[1], ibuf, strlen(ibuf));
			}

		if (close(opfd[0]) == -1 || close(opfd[1]) == -1 ) { /* no longer needed */
			perror("close2");
			exit(1);
		}
		if (close(0) == -1) {		/* make fd 0 (stdin) available */
			perror("close0");
			exit(1);
		}
		if (dup(ipfd[0]) != 0) {	/* make stdin input pipe */
			perror("idup0");
			exit(1);
		}
		if (close(ipfd[0]) == -1 || close(ipfd[1]) == -1 ) { /* no longer needed */
			perror("close2");
			exit(1);
		}

		while (fgets(buf, BUFSIZ, stdin) != NULL) {
			if (strncmp(action, "shutdown", 8) == 0) {
				if (strncmp((char *)buf, MSG_SHUTDOWN, 25) == 0) {
					success = 1;
					buf[strlen((char *)buf)-1] = '\0';
				} else
					printf("%s", (char *)buf);
			} else if (strncmp(action, "startup", 7) == 0) {
				if (strcmp(action, "startup nomount") == 0) {
					if (strncmp((char *)buf, MSG_STARTED, 23) == 0) {
						success = 1;
						buf[strlen((char *)buf)-1] = '\0';
					} else
						printf("%s", (char *)buf);
				} else if (strcmp(action, "startup mount") == 0) {
					if (strncmp((char *)buf, MSG_MOUNTED, 16) == 0) {
						success = 1;
						buf[strlen((char *)buf)-1] = '\0';
					} else
						printf("%s", (char *)buf);
				} else if (strcmp(action, "startup open") == 0 || strcmp(action, "startup") == 0) {
					if (strncmp((char *)buf, MSG_OPENED, 15) == 0) {
						success = 1;
						buf[strlen((char *)buf)-1] = '\0';
					} else
						printf("%s", (char *)buf);
				}
			} else
				printf("%s", (char *)buf);
		}

		wait(&status);
#endif
	} else if (!fnmatch("*/appsutil/*", p->path, 0)) {
		/* script - pass args on command line */
#ifdef NOEX
		command = argcat("sh", p->path);	/* assume script not executable */
#else
		command = strdup(p->path);	/* assume script is executable */
#endif
		command = argcat(command, action);
		if (nflg) 
			printf("%s%s\n", command, (getuid() == 0 && p->uid != (uid_t)0) ?
				"\"" : "");
		 else
			status = system(command);
	} else {
		/* sqldba needs arguments to be quoted */
		if (p->args[2]) {
			command = (char *)malloc(strlen(p->path) + strlen(p->args[0]) +
					strlen(p->args[1]) + strlen(p->args[2]) + 5);
			sprintf(command, "%s %s\"%s %s\"\n",
				p->path, p->args[0], p->args[1], p->args[2]);
		} else {
			command = (char *)malloc(strlen(p->path) + strlen(p->args[0]) +
					strlen(p->args[1]) + 5);
			sprintf(command, "%s %s%s\n",
				p->path, p->args[0], p->args[1]);
		}
		if (nflg)
			printf("%s%s\n", command, (getuid() == 0 && p->uid != (uid_t)0) ?
				"\"" : "");
		else
			status = system(command);
	}

	setuid(0);

	free(command);

	if (opt)
		free(action);

	if (WIFEXITED(status)) { /* child terminated normally */
		if (success)
			return 0;
		else
			return WEXITSTATUS(status);
	} else
		return 1;
#ifdef USE_POPEN
piperr:
	perror("pipe");
	return(errno);
#endif
}
