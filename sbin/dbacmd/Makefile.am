# $Header$

noinst_HEADERS = dbopts.h

sbin_PROGRAMS = dbacmd

dbacmd_SOURCES = dbacmd.c dbopts.c
dbacmd_CFLAGS = -I../../include -DAPPS
dbacmd_LDFLAGS = -L../../lib/dbinfo
dbacmd_LDADD = ../../lib/libdbinfo.la
