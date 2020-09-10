#ident $Header$
AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libdbinfo.la

libdbinfo_la_SOURCES = dbinfo/getcmd.c dbinfo/getscript.c dbinfo/dbinfo.c dbinfo/arg.c dbinfo/setcmd.c dbinfo/getctx.c
libdbinfo_la_CFLAGS = -I../include
libdbinfo_la_LDFLAGS = -version-info 1:1:1

if LINUX
libdbinfo_la_SOURCES += dbinfo/bufsplit.c
endif

if AIX
 libdbinfo_la_SOURCES += dbinfo/bufsplit.c dbinfo/gethost_aix.c
 libdbinfo_la_CFLAGS += -O3 -q64 -qtls -DSS_64BIT_SERVER -qwarn64 -qinfo=uni -DAIXRIOS -qflag=e:e -qsuppress=1506-159 -qsuppress=1506-010 -Q! -Wl,-b64
 libdbinfo_la_LDFLAGS += -b64 -lhmu -bI:/usr/lib/iocp.exp -lc_r
else
 libdbinfo_la_SOURCES += dbinfo/gethost.c
endif

#liboratab_la_SOURCES = oratab/oratab.c
#liboratab_la_CFLAGS = 
#liboratab_la_LDFLAGS = -version-info 1:1:1
