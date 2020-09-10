#!/bin/sh -
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

if expr "$prog" : '.*oper' >&-
then
	index=1
fi

config=$ORACLE_HOME/rdbms/lib/config.o
if [ ! -r $config ]
then
	echo "$prog: cannot read '$config'" >&2
	exit 1
fi

tmpfile=`mktemp -c -p $prog`

cat - > $tmpfile.c <<-!
	#include <stdio.h>
	extern char *ss_dba_grp[];
	int main()
	{
		printf("%s\n", ss_dba_grp[${index:-0}]);
		exit(0);
	}
!

cc +DD64 -o $tmpfile $tmpfile.c $config
$tmpfile

rm -f $tmpfile $tmpfile.c

exit 0
