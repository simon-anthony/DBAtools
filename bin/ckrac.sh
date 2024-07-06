#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
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
PATH=/usr/bin export PATH

prog=`basename $0`

usage() {
	cat <<!
usage: $prog {-h <oracle_home> | -s <sid> [-o]}
!
[ $help ] && cat <<!

   -h <oracle_home>  Set the ORACLE_HOME directly

   -s <sid>          Use <sid> to determine the ORACLE_HOME

   -o                If using -s, use oratab rather than the database context
                     file.

!
}

while getopts s:h:o opt 2>&-
do
	case $opt in
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
[ ! "$sflg" -a ! "$hflg" ] && errflg=y	# at least one of -h or -s required

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
		. BINDIR/adenv -do "$sid" || exit $?
	fi
fi

mkfile=$ORACLE_HOME/rdbms/lib/env_rdbms.mk

if [ ! -r $mkfile ]
then	
	echo "$prog: cannot read '$mkfile'" >&2
	exit 1
fi

rac_on=`sed -n '/RAC_ON[ 	]*=/ s;.*=[ 	]*\([^ 	\.]*\)\..*;\1;p' $mkfile`
rac_off=`sed -n '/RAC_OFF[ 	]*=/ s;.*=[ 	]*\([^ 	\.]*\)\..*;\1;p' $mkfile`

newest=`ls -t $ORACLE_HOME/rdbms/lib/libknlopt.a $ORACLE_HOME/bin/oracle | head -1`
if [ "$newest" != $ORACLE_HOME/bin/oracle ]
then
	echo "$prog: WARNING oracle kernel has yet to be compiled after change to RAC option" >&2
fi

[ $hflg ] || print -n "$ORACLE_HOME "

if ar t $ORACLE_HOME/rdbms/lib/libknlopt.a | grep -qs $rac_on\\.o 
then
	echo true
	exit 0
fi

if ar t $ORACLE_HOME/rdbms/lib/libknlopt.a | grep -qs $rac_off\\.o
then
	echo false
	exit 0
fi

echo "$prog: cannot determine whether RAC is on or off" >&2

exit 1
