#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# racp: return package name associated with given sid
#       or list of databases with SGeRAC package configured
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
PATH=/bin:/usr/bin export PATH

prog=`basename $0`
 
usage() {
	cat <<!
usage: $prog [-s <sid>]
!
[ $help ] && cat <<!

   -s <sid>  The name of the database.

!
}

while getopts s: opt 2>&-
do
	case $opt in
	s)	sid="$OPTARG"
		sflg=y						# SID
		unset TWO_TASK
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 0 ] || errflg=y
 
[ $errflg ] && { usage >&2; exit 2; }

if ls /etc/cmcluster/*/rac_dbi.conf >/dev/null 2>&1
then
	:
else
	exit $prog: no SGeRAC instances configured >&2
	exit 1
fi

if [ $sflg ]
then
	conf=`awk -F= '$1 == "ORACLE_DBNAME" && $2 == "'$sid'" { print FILENAME }' \
		/etc/cmcluster/*/rac_dbi.conf 2>&-`
	[ -r "$conf" ] || exit 1
	basename `ls \`dirname $conf\`/*.ctl` .ctl
else
	awk -F= '$1 == "ORACLE_DBNAME" { print $2 }' /etc/cmcluster/*/rac_dbi.conf
fi

exit 0
