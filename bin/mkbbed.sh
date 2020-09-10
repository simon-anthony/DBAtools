#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# unres: find unresolved library references for a.out file
#
################################################################################
# Copyright (C) 2008 Simon Anthony
# 
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License or, (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program. If not see <http://www.gnu.org/licenses/>>
# 
PATH=/bin:/usr/bin:/usr/ccs/bin:BINDIR export PATH

prog=`basename $0`
 
usage() {
	cat <<!
usage: $prog [-f] [-h <oracle_home> | -s <sid> [-o]]
!
[ $help ] && cat <<!

   -f                Force rebuiling of bbed even if it exists

   -h <oracle_home>  Set the ORACLE_HOME directly

   -s <sid>          Use <sid> to determine the ORACLE_HOME

   -o                If using -s, use oratab rather than the database context
                     file.

!
}

while getopts fh:s:o opt 2>&-
do
	case $opt in
	f)	fflg=y				# force rebuild
		;;
	s)	sid="$OPTARG"
		sflg=y
		[ $hflg ] && errflg=y
		;;
	h)	ORACLE_HOME="$OPTARG"
		export ORACLE_HOME
		hflg=y
		[ $sflg ] && errflg=y
		;;
	o)	oflg=y				# use oratab entry
		;;
	\?)	errflg=y
		help=y
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 0 ] || errflg=y
[ "$oflg" -a ! "$sflg" ] && errflg=y	# -s required for -o

[ $errflg ] && { usage >&2; exit 2; }

if [ $sflg ]
then
	if [ $oflg ]
	then
		oratab=`ls /var/opt/oracle/oratab /etc/oratab 2>&-`
		[ $oratab ] || { echo "$prog: cannot find oratab" >&2; exit 1; }
		ORACLE_HOME=`awk -F: '$1 == "'$sid'" { print $2 }' $oratab`
		[ $ORACLE_HOME ] || {
			echo "$prog: cannot find '$sid' in $oratab" >&2; exit 1; }
	else
		if dbctxfile=`getctx -d -s $sid`
		then
			ORACLE_HOME=`ctxvar s_db_oh $dbctxfile`
			export ORACLE_HOME
		else
			print -u2 "$prog: cannot find context file for '$sid'"
			exit 1
		fi
	fi
fi

[ $ORACLE_HOME ] || { echo $prog: ORACLE_HOME must be set >&2; exit 1; }

ckperm() {
	if [ ! -w $ORACLE_HOME/rdbms/lib ]
	then
		print -u2 "$prog: no permission to write to $ORACLE_HOME/rdbms/lib"
		exit 1
	fi
}
vers=`ORACLE_HOME=$ORACLE_HOME SBINDIR/dbvers -n1` || exit $?

if [ $vers -ge 10 ]
then
	if [ ! -r $ORACLE_HOME/rdbms/lib/sbbdpt.o -o \
		 ! -r $ORACLE_HOME/rdbms/lib/ssbbded.o ]
	then
		ckperm
		ln -f -s LIBDIR/sbbdpt.o $ORACLE_HOME/rdbms/lib/sbbdpt.o
		ln -f -s LIBDIR/ssbbded.o $ORACLE_HOME/rdbms/lib/ssbbded.o
		ln -f -s DATADIR/mesg/bbedus.msb $ORACLE_HOME/rdbms/mesg/bbedus.msb
		ln -f -s DATADIR/mesg/bbedus.msg $ORACLE_HOME/rdbms/mesg/bbedus.msg
	fi
fi

rebuild=
if [ -x $ORACLE_HOME/rdbms/lib/bbed ]
then
	if ckres -q $ORACLE_HOME/rdbms/lib/bbed
	then
		[ $fflg ] && rebuild=y
	else
		rebuild=y
	fi
else
	rebuild=y
fi

if [ $rebuild ]
then
	ckperm
	rm -f $ORACLE_HOME/rdbms/lib/bbed
	cd $ORACLE_HOME/rdbms/lib
	if [ ! -r ins_rdbms.mk ]
	then	
		echo "$prog: cannot read '$mkfile'" >&2
		exit 1
	fi
	make -f ins_rdbms.mk $ORACLE_HOME/rdbms/lib/bbed
else
	exit 0
fi
