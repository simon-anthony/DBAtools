# $Header$

if HPUX
bin_PROGRAMS = pgrep
endif
if AIX
bin_PROGRAMS = pgrep
bin_PROGRAMS += ptree
endif

if AIX
pgrep_SOURCES = pgrep_aix.c str2sig.c
endif
if HPUX
pgrep_SOURCES = pgrep_hpux.c str2sig.c
endif
pgrep_CFLAGS =
pgrep_LDADD = 

if AIX
ptree_SOURCES = ptree_aix.c
endif
