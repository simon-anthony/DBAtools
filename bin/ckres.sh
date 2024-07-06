#!/usr/bin/sh -
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
#unset SHLIB_PATH LD_LIBRARY_PATH ORACLE_HOME CONETXT_FILE

prog=`basename $0`
 
name=`expr "$prog" : 'ad\(.*\)$' 2>&-`

usage="[-q] [-s] [-n] <file>"

while getopts qsn opt 2>&-
do
	case $opt in
	q)	qflg=y					# quiet
		;;
	s)	sflg=y					# ignore non-existent files
		;;
	n)	nflg=y					# do not reset SHLIB_PATH or LD_LIBRARY_PATH
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 1 ] || errflg=y
 
[ $errflg ] && { echo usage: $prog $usage >&2; exit 2; }

if [ ! "$nflg" ]
then
	unset SHLIB_PATH LD_LIBRARY_PATH
fi

ldd -v "$1" 2>&1 | awk -v qflg=${qflg:+1} '
/ldd: cannot open/ {
	r = 3
	exit
}
/find library=/ {
	lib = substr($0, index($0, "=") + 1, index($0, ";") - index($0, "=") - 1)
}
/(Can'"'"'t open shared library)|(Unable to find library)/ {
	ulib = $(NF)
	gsub("'"'"'", "", ulib) 
	sub("\.$", "", ulib)
	if (ulib == lib)
		if (!qflg)
			print "lib is " lib
	r = 2
}
END {
	exit r
}
'
case $? in
3)	[ $sflg ] || print -u2 $prog: cannot open $1;
	exit 1
	;;
1)	print -u2 $prog: cannot open $1;
	exit 1
	;;
0)	exit 0
	;;
?)	exit 1
esac
