#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

/* Types */
#define ORA_TOP		1
#define COMMON_TOP	2
#define APPL_TOP	3
#define DB_TOP		4
#define ARCH_TOP	5
#define INST_TOP	6
#define DATA_TOPS	7

#define	MAXPAT		20
#define	ETCDIR		"/etc/opt/oracle"
#define	PATFILE		ETCDIR "/rwpat"
#define	PATHFILE	ETCDIR "/rwtop" 

#define	RW_SILENT		0x001
#define	RW_UPDATE		0x002
#define	RW_NOLOOKUP		0x004
#define	RW_NORESOLV		0x008
#define	RW_FIRST		0x010

#define RW_ISSILENT(mode)	((mode)&RW_SILENT)
#define RW_ISUPDATE(mode)	((mode)&RW_UPDATE)
#define RW_ISNOLOOKUP(mode)	((mode)&RW_NOLOOKUP)
#define RW_ISNORESOLV(mode)	((mode)&RW_NORESOLV)
#define RW_ISFIRST(mode)	((mode)&RW_FIRST)

struct topstr {
	char	*gtop;
	char	*sfx;
	char	*var;
	int		flg;
};

#ifdef _RWTOP

struct pat {
	int		flg;
	char	*globpat;
};

static struct pat defpat[] = {
	{ COMMON_TOP, "/[du][0-9][0-9]/oracle/${s_systemname_lower}comn" },
	{ ORA_TOP,    "/[du][0-9][0-9]/oracle/${s_systemname_lower}ora" },
	{ DB_TOP,     "/[du][0-9][0-9]/oracle/${s_systemname_lower}db" },
	{ APPL_TOP,   "/[du][0-9][0-9]/oracle/${s_systemname_lower}appl" },
	{ INST_TOP,   "/[du][0-9][0-9]/oracle/${s_systemname_lower}inst" },
	{ ARCH_TOP,   "/[du][0-9][0-9]/oracle/${s_systemname_lower}arch" },
	{ DATA_TOPS,  "/[du][0-9][0-9]/oracle/${s_systemname_lower}data" },

	{ COMMON_TOP, "/[du][0-9][0-9]/app/${s_systemname}/comn" },
	{ ORA_TOP,    "/[du][0-9][0-9]/app/${s_systemname}/oracle" },
	{ DB_TOP,     "/[du][0-9][0-9]/app/${s_systemname}/oracle" },
	{ APPL_TOP,   "/[du][0-9][0-9]/app/${s_systemname}/apps" },
	{ INST_TOP,   "/[du][0-9][0-9]/app/${s_systemname}/inst" },
	{ ARCH_TOP,   "/[du][0-9][0-9]/app/${s_systemname}/arch" },
	{ DATA_TOPS,  "/[du][0-9][0-9]/oradata/${s_systemname}" },

	{ COMMON_TOP, "/apps/${s_systemname}/apps/apps_st/comn" },
	{ ORA_TOP,    "/apps/${s_systemname}/apps/tech_st" },
	{ DB_TOP,     "/apps/${s_systemname}/db/tech_st" },
	{ APPL_TOP,   "/apps/${s_systemname}/apps/apps_st/appl" },
	{ INST_TOP,   "/apps/${s_systemname}/inst/apps/{ctx}" },
	{ ARCH_TOP,   "/apps/oracle/${s_systemname}/inst/apps/{ctx}/logs" },
	{ DATA_TOPS,  "/apps/oracle/${s_systemname}/db/apps_st/data" },

	{ COMMON_TOP, "/apps/${s_systemname}/apps/apps_st/comn" },
	{ ORA_TOP,    "/apps/${s_systemname}/fs1/EBSapps" },
	{ DB_TOP,     "/apps/${s_systemname}/db/tech_st" },
	{ APPL_TOP,   "/apps/${s_systemname}/fs1/EBSapps/appl" },
	{ INST_TOP,   "/apps/${s_systemname}/fs1/inst/apps/{ctx}" },
	{ ARCH_TOP,   "/apps/oracle/${s_systemname}/inst/apps/{ctx}/logs" },
	{ DATA_TOPS,  "/apps/${s_systemname}/oradata" },

	{ COMMON_TOP, "/apps/${s_systemname}/*/apps/apps_st/comn" },
	{ ORA_TOP,    "/apps/${s_systemname}/*/fs1/EBSapps" },
	{ DB_TOP,     "/apps/${s_systemname}/*/db/tech_st" },
	{ APPL_TOP,   "/apps/${s_systemname}/*/fs1/EBSapps/appl" },
	{ INST_TOP,   "/apps/${s_systemname}/*/fs1/inst/apps/{ctx}" },
	{ ARCH_TOP,   "/apps/oracle/${s_systemname}/*/inst/apps/{ctx}/logs" },
	{ DATA_TOPS,  "/apps/${s_systemname}/*/oradata" },

	{ COMMON_TOP, "/[du][0-9][0-9]/oracle/${s_systemname}/apps/apps_st/comn" },
	{ ORA_TOP,    "/[du][0-9][0-9]/oracle/${s_systemname}/apps/tech_st" },
	{ DB_TOP,     "/[du][0-9][0-9]/oracle/${s_systemname}/db/tech_st" },
	{ APPL_TOP,   "/[du][0-9][0-9]/oracle/${s_systemname}/apps/apps_st/appl" },
	{ INST_TOP,   "/[du][0-9][0-9]/oracle/${s_systemname}/inst/apps/{ctx}" },
	{ ARCH_TOP,   "/[du][0-9][0-9]/oracle/${s_systemname}/inst/apps/{ctx}/logs" },
	{ DATA_TOPS,  "/[du][0-9][0-9]/oracle/${s_systemname}/db/apps_st/data" },
	0
};

struct topstr tops[] = {
	{ "ora*",  "ora",  "ORA_TOP",    ORA_TOP },
	{ "com*",  "comn", "COMMON_TOP", COMMON_TOP },
	{ "app*",  "appl", "APPL_TOP",   APPL_TOP },
	{ "db*",   "db",   "DB_TOP",     DB_TOP },
	{ "arc*",  "arch", "ARCH_TOP",   ARCH_TOP },
	{ "ins*",  "inst", "INST_TOP",   INST_TOP },
	{ "dat*",  "data", "DATA_TOPS",  DATA_TOPS },
	NULL
};
#endif

#ifndef _RWTOP
#ifdef __STDC__
extern int	rwtyp(char *);
extern char	**rwtop(char *pat[], int, char *, int, char *, int);
extern int	rwchk(int, char *);
#else
extern int	rwtyp();
extern char	**rwtop();
extern int	rwchk();
#endif
#endif
