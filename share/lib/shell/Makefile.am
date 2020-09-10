# $Header$

datalibdir = $(datadir)/lib

SUFFIXES = .sh .m4

.m4.sh:
	@m4 -DBASEDIR=$(prefix) \
	-DSBINDIR=$(sbindir) \
	-DBINDIR=$(bindir) \
	-DLIBDIR=$(libdir) \
	-DDATADIR=$(datadir) \
	-DSYSCONFDIR=$(sysconfdir) \
	-DPREFIX=$(prefix) \
	$< > $@

datalib_SCRIPTS = \
	adstart.sh \
	agentstart.sh \
	dbshut.sh \
	dbstart.sh \
	lsnrstart.sh \
	opmnstart.sh

CLEANFILES = $(datalib_SCRIPTS)

EXTRA_DIST = \
	adstart.m4 \
	agentstart.m4 \
	dbshut.m4 \
	dbstart.m4 \
	lsnrstart.m4 \
	opmnstart.m4
