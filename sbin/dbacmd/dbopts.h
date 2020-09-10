#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

#define USAGE_DBSHUT	"[-nd] [immediate|abort|normal|transactional [local]]"

#define USAGE_DBSTART	"[-nd] [force] [restrict] [pfile=<file>] [quiet] [mount|open [read {only|write}]|nomount]"


#define GET		1
#define SET		2
#define PRINT	3

#ifndef _DBOPTS
#ifdef __STDC__
extern int chkopt(char *, int);
extern int chkopt(char *, int);
extern int valopt(void);
extern char *stropt(void);
extern void setprog(char *);
#else
extern int chkopt();
extern int chkopt();
extern int valopt();
extern char *stropt();
extern void setprog();
#endif
#else
#define DBSTART	0
#define DBSHUT	1
#define IS_PROG(n) prg == n
#define IS_DBSTART	IS_PROG(DBSTART)
#define IS_DBSHUT	IS_PROG(DBSHUT)

struct option {
	char	*name;
	char	*value;
	int		flg;
};
struct option startopts[] = {
	{ "force", NULL, 0},
	{ "restrict", NULL, 0},
	{ "quiet", NULL, 0},
	{ "pfile", NULL, 0},
	{ "nomount", NULL, 0},
	{ "mount", NULL, 0},
	{ "open", NULL, 0},
	{ "read", NULL, 0},
	{ "only", NULL, 0},
	{ "write", NULL, 0},
	NULL
};
struct option shutopts[] = {
	{ "immediate", NULL, 0},
	{ "abort", NULL, 0},
	{ "normal", NULL, 0},
	{ "transactional", NULL, 0},
	{ "local", NULL, 0},
	NULL
};
#endif
