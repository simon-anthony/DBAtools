# $Header$
# Newer releases of Oracle server for not have the VERSION suffix in library names

#VERSION = 12
VERSION = 
bin_PROGRAMS = adsvcctl

adsvcctl_SOURCES = adsvcctl.c
adsvcctl_CFLAGS = -I$(ORACLE_HOME)/xdk/include -DORA_NLS10=\"$(datadir)/nls/data\"
adsvcctl_LDFLAGS = -L$(ORACLE_HOME)/lib 
if AIX
adsvcctl_CFLAGS += -O3 -q64 -qtls -DSS_64BIT_SERVER -qwarn64 -qinfo=uni -DAIXRIOS -qflag=e:e -qsuppress=1506-159 -qsuppress=1506-010 -Q! 
adsvcctl_LDFLAGS += -b64 -lhmu -bI:/usr/lib/iocp.exp -lc_r
endif
adsvcctl_LDADD = -lxml$(VERSION) -lcore$(VERSION) -lnls$(VERSION) -lunls$(VERSION) -lcore$(VERSION) -lnls$(VERSION) -lcore$(VERSION) -lnls$(VERSION) -lm -lpthread -lclntsh
adsvcctl_LIBTOOLFLAGS = --preserve-dup-deps
