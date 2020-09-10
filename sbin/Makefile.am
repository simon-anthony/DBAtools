# $Header$

SUBDIRS = dbacmd 

if HPUX
SUBDIRS += cmmaintpkg.d
endif

sbin_PROGRAMS = adbatch oratab

adbatch_SOURCES = adbatch.c
oratab_SOURCES = oratab.c

adbatch_CFLAGS =
oratab_CFLAGS =

adbatch_LDADD =
oratab_LDADD = 


SUFFIXES = .sh

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

sbin_SCRIPTS = dbstart
if HPUX
sbin_SCRIPTS += cmmaintpkg cmvgpkg
endif

CLEANFILES = $(sbin_SCRIPTS)

EXTRA_DIST = dbstart.sh
if HPUX
EXTRA_DIST += cmmaintpkg.sh cmvgpkg.sh
endif
