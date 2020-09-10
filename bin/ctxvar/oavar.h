#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid_oavar_h = "$Header$";
#endif
#ifdef __STDC__
extern char *getoavar(const char *name, const char *path);
extern char *setoavar(const char *name, const char *value, const char *path);
#else
extern char *getoavar();
extern char *setoavar();
#endif
