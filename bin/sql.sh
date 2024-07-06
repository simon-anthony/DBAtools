#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
ORACLE_PATH=DATADIR/lib/sql:$ORACLE_PATH
SQLPATH=DATADIR/lib/sql:$SQLPATH
export ORACLE_PATH SQLPATH

if [ "X$ORACLE_HOME" = "X" ]
then
	echo "ORACLE_HOME not set" >&2
	exit 1
elif [ ! -x "$ORACLE_HOME/bin/sqlplus" ]
then
	echo "sqlplus: not found" >&2
	exit 1
fi

exec sqlplus "$*"

echo "sqlplus: exec error" >&2
exit 1
