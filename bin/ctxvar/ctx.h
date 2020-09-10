#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid_h = "$Header$";
#endif

#define CTX_PRINT	0x001
#define CTX_REGEX	0x002
#define CTX_EDIT	0x004
#define CTX_PRINTV	0x008

#define CTX_ISPRINT(mode)	((mode)&CTX_PRINT)
#define CTX_ISPRINTV(mode)	((mode)&CTX_PRINTV)
#define CTX_ISREGEX(mode)	((mode)&CTX_REGEX)
#define CTX_ISEDIT(mode)	((mode)&CTX_EDIT)

#define CTX_VAR		1
#define CTX_CURVAL	2
#define CTX_NEWVAL	3
#define CTX_FOUND	4

#define MAXVARS 1024
typedef struct variable {
	char *name;
	char *value;
} Variable;

#ifndef CTX
extern void ctxsetflags(int);
//extern void ctxset(int, const char *);
//extern void ctxset(int, const Variable *v[]);
extern void ctxset(int, Variable *v[]);
extern void ctxsetedit(int);
extern char *ctxget(int);
extern int ctxfound(void);
#endif

