# $Header$
SUBDIRS = ps adsvcctl ctxvar

AUTOMAKE_OPTIONS = subdir-objects

noinst_HEADERS = rw.h

bin_PROGRAMS = rwtop rwid getgrname gethostnames nbread pset realpath

if AIX
bin_PROGRAMS += mktemp dos2ux
endif

xlc64_CFLAGS = -O3 -q64 -qtls -DSS_64BIT_SERVER -qwarn64 -qinfo=uni -DAIXRIOS -qflag=e:e -qsuppress=1506-159 -qsuppress=1506-010 -Q! 
xlc64_LDFLAGS = -lhmu -bI:/usr/lib/iocp.exp -lc_r

rwtop_SOURCES = rwtop.c rwchk.c
rwtop_CFLAGS =
rwtop_LDADD = librw.la
if AIX
rwtop_CFLAGS += $(xlc64_CFLAGS)
rwtop_LDFLAGS = $(xlc64_LDFLAGS)
endif

rwid_SOURCES = rwid.c
rwid_CFLAGS =
rwid_LDADD = librw.la
if AIX
rwid_CFLAGS += $(xlc64_CFLAGS)
rwid_LDFLAGS = $(xlc64_LDFLAGS)
endif

noinst_LTLIBRARIES = librw.la
librw_la_SOURCES = rw.c gsub.c getcontexts.c param.c
if AIX
 librw_la_SOURCES += ../lib/dbinfo/gethost_aix.c
else
 librw_la_SOURCES += ../lib/dbinfo/gethost.c
endif
librw_la_CFLAGS =
if SUN
librw_la_LDFLAGS = -lgen
endif
if LINUX
librw_la_LDFLAGS =
endif
if AIX
librw_la_CFLAGS += $(xlc64_CFLAGS)
librw_la_LDFLAGS = -bstatic -bsno $(xlc64_LDFLAGS)
endif

getgrname_SOURCES = getgrname.c
getgrname_CFLAGS = 
getgrname_LDFLAGS =
if AIX
 getgrname_CFLAGS += $(xlc64_CFLAGS)
 getgrname_LDFLAGS += $(xlc64_LDFLAGS)
endif

gethostnames_SOURCES = gethostnames.c
gethostnames_CFLAGS = 
gethostnames_LDFLAGS =
if AIX
 gethostnames_CFLAGS += $(xlc64_CFLAGS)
 gethostnames_LDFLAGS += $(xlc64_LDFLAGS)
 gethostnames_SOURCES += ../lib/dbinfo/gethost_aix.c
else
 gethostnames_SOURCES += ../lib/dbinfo/gethost.c
endif

nbread_SOURCES = nbread.c
nbread_CFLAGS = 
nbread_LDFLAGS =
if AIX
 nbread_CFLAGS += $(xlc64_CFLAGS)
 nbread_LDFLAGS += $(xlc64_LDFLAGS)
endif

pset_SOURCES = pset.c
pset_CFLAGS = 
pset_LDFLAGS = 
if AIX
 pset_CFLAGS += $(xlc64_CFLAGS)
 pset_LDFLAGS += $(xlc64_LDFLAGS) -lcurses
endif

realpath_SOURCES = realpath.c
realpath_CFLAGS = 
realpath_LDFLAGS =
if AIX
 realpath_CFLAGS += $(xlc64_CFLAGS)
 realpath_LDFLAGS += $(xlc64_LDFLAGS)
endif

if AIX
mktemp_SOURCES = mktemp.c
mktemp_CFLAGS = $(xlc64_CFLAGS)
mktemp_LDFLAGS = $(xlc64_LDFLAGS)
endif

SUFFIXES = .sh .pl

.sh:
	@echo "g/BASEDIR/s;BASEDIR;$(prefix);g" > exscript ;\
	echo "g/SBINDIR/s;SBINDIR;$(sbindir);g" >> exscript ;\
	echo "g/BINDIR/s;BINDIR;$(bindir);g" >> exscript ;\
	echo "g/LIBDIR/s;LIBDIR;$(libdir);g" >> exscript ;\
	echo "g/DATADIR/s;DATADIR;$(datadir);g" >> exscript ;\
	echo "g/SYSCONFDIR/s;SYSCONFDIR;$(sysconfdir);g" >> exscript ;\
	echo "g/LOCALSTATEDIR/s;LOCALSTATEDIR;$(localstatedir);g" >> exscript ;\
	echo "g/PREFIX/s;PREFIX;$(prefix);g" >> exscript ;\
	echo "w! $@" >> exscript 
	@ex -s $< < exscript
	@rm -f exscript
	@chmod +x $@

.pl:
	@echo "g/BASEDIR/s;BASEDIR;$(prefix);g" > exscript ;\
	echo "g/SBINDIR/s;SBINDIR;$(sbindir);g" >> exscript ;\
	echo "g/BINDIR/s;BINDIR;$(bindir);g" >> exscript ;\
	echo "g/LIBDIR/s;LIBDIR;$(libdir);g" >> exscript ;\
	echo "g/DATADIR/s;DATADIR;$(datadir);g" >> exscript ;\
	echo "g/SYSCONFDIR/s;SYSCONFDIR;$(sysconfdir);g" >> exscript ;\
	echo "g/LOCALSTATEDIR/s;LOCALSTATEDIR;$(localstatedir);g" >> exscript ;\
	echo "g/PREFIX/s;PREFIX;$(prefix);g" >> exscript ;\
	echo "w! $@" >> exscript 
	@ex -s $< < exscript
	@rm -f exscript
	@chmod +x $@

bin_SCRIPTS = \
	256 \
	colourtest \
	adcfg \
	adcpusr \
	adctl \
	adcustom \
	adenv \
	adinstcustom \
	adpasswd \
	adpre \
	adsessions \
	adstatus \
	asmdisk \
	chkinv \
	ckdba \
	ckint \
	ckitem \
	ckpath \
	ckrac \
	ckres \
	ckyorn \
	crsstat \
	ctx \
	dbh \
	getctx \
	getprb \
	mkdefs \
	sql \
	sshping 

if HPUX 
bin_SCRIPTS += ptree racp
endif

CLEANFILES = $(bin_SCRIPTS)

EXTRA_DIST = \
	256.pl \
	colourtest.pl \
	adcfg.sh \
	adcpusr.sh \
	adctl.sh \
	adcustom.sh \
	adenv.sh \
	adinstcustom.sh \
	adpasswd.sh \
	adpre.sh \
	adsessions.sh \
	adstatus.sh \
	asmdisk.sh \
	chkinv.sh \
	ckdba.sh \
	ckint.sh \
	ckitem.sh \
	ckpath.sh \
	ckrac.sh \
	ckres.sh \
	ckyorn.sh \
	crsstat.sh \
	ctx.sh \
	dbh.sh \
	getctx.sh \
	getprb.sh \
	mkdefs.sh \
	mkenv.sh \
	mkscripts.sh \
	sql.sh \
	sshping.sh 

if HPUX
EXTRA_DIST += ptree.sh racp.sh
endif
