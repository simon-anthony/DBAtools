#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

struct cmdstr {
	char	path[PATH_MAX];				/* command for startup/shutdown */
	char	*oracle_sid;
	char	*oracle_home;
	char	*version;					/* database version */
	char	*connect;					/* database user and connect string */
	uid_t	uid;
	gid_t	gid;
	char	*name;
	char	*dir;
	char	*args[ARG_MAX];
};

#ifndef _DBINFO
#ifdef __STDC__
extern struct cmdstr * dbinfo(struct cmdstr *, const char *);
#else
extern struct cmdstr * dbinfo();
#endif
#endif

#define T_SQLDBA		0				/* test mode for sql command */
#define T_SQLPLUS		1
#define R_COMMAND		2				/* no login */
#define R_NOLOG			3
										/* arguments to sqldba for no action */
static char	*cmdargs[] = { "command=exit", "-\?", "command=", "/nolog" };

#define TEST(t)			cmdargs[t]
#define CMDLINE(t)		cmdargs[t]

#define INTERNAL	0
#define SYSDBA		1

static char *users[] = { "internal", "/ as sysdba" };

#define USER(u) users[u]

#define	CMD_ARGV	0					/* args taken from command line */
#define	CMD_STDIN	1					/* args taken from stdin */
										/* users */
