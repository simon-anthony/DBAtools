#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
# Provided in case you have 10g+ which does not include bbed libs
PATH=/usr/bin:/bin export PATH

[ $ORACLE_HOME ] || { echo $prog: ORACLE_HOME must be set >&2; exit 1; }
if [ ! -w $ORACLE_HOME/rdbms/lib ]
then
	print -u2 "lnbbed: no permission to write to $ORACLE_HOME/rdbms/lib"
	exit 1
fi
vers=`ORACLE_HOME=$ORACLE_HOME SBINDIR/dbvers -n1` || exit $?

if [ $vers -ge 10 ]
then
	ln -f -s LIBDIR/sbbdpt.o $ORACLE_HOME/rdbms/lib/sbbdpt.o
	ln -f -s LIBDIR/ssbbded.o $ORACLE_HOME/rdbms/lib/ssbbded.o
	ln -f -s DATADIR/mesg/bbedus.msb $ORACLE_HOME/rdbms/mesg/bbedus.msb
	ln -f -s DATADIR/mesg/bbedus.msg $ORACLE_HOME/rdbms/mesg/bbedus.msg
fi

exit 0
