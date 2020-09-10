# $Header$

BUILT_SOURCES = parser.h
AM_YFLAGS = -d 
AM_LFLAGS = 
bin_PROGRAMS = ctxvar

ctxvar_SOURCES = parser.y lexer.l main.c
include_HEADERS = oavar.h
noinst_HEADERS = ctx.h

ctxvar_CFLAGS = 
ctxvar_LDFLAGS = 
if AIX
ctxvar_CFLAGS += -O3 -q64 -qtls -DSS_64BIT_SERVER -qwarn64 -qinfo=uni -DAIXRIOS -qflag=e:e -qsuppress=1506-159 -qsuppress=1506-010 -Q! -Wl,-b64
ctxvar_LDFLAGS += -b64 -lhmu -bI:/usr/lib/iocp.exp -lc_r
endif

lib_LTLIBRARIES = liboavar.la

liboavar_la_SOURCES = lexer.c parser.c oavar.c lock.c
liboavar_la_CFLAGS = -I .
liboavar_la_LDFLAGS = -version-info 1:1:1

CLEANFILES = lexer.c parser.c parser.h

