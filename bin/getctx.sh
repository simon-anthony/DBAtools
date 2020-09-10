#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# getctx: find CTX file 
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
PATH=/bin:/usr/bin:BINDIR export PATH
FPATH=BASEDIR/share/lib/functions export FPATH

prog=`basename $0`
usage="[-a[-r|-p]|-d [-v<n>]] -f <path to context file> | -s <s_systemname> [-D <dir>] [-A <dir>]"
option=-a	# default
header=adxmlctx
type=applications

while getopts aA:dD:f:v:s:rp opt 2>&-
do
	case $opt in
	f)	ctxfile="$OPTARG"
		fflg=y
		;;
	a)	aflg=y
		option=-a
		header=adxmlctx
		type=applications
		;;
	A)	Aflg=y
		APPL_TOP=$OPTARG
		[ -d "$APPL_TOP" -a -x "$APPL_TOP" -a -r "$APPL_TOP" ] || {
			echo $prog: cannot access $APPL_TOP; exit 1;
		}
		[ $fflg ] && errflg=y
		;;
	d)	dflg=y
		option=-d
		header=adxdbctx
		type=database
		;;
	D)	Dflg=y
		DB_TOP=$OPTARG
		[ -d "$DB_TOP" -a -x "$DB_TOP" -a -r "$DB_TOP" ] || {
			echo $prog: cannot access $DB_TOP; exit 1;
		}
		[ $fflg ] && errflg=y
		;;
	v)	vflg=y
		expr "$OPTARG" : '[0-9]\{1,2\}$' >&- || errflg=y
		voption="-v $OPTARG"
		;;
	s)	s_systemname="$OPTARG"
		sflg=y
		;;
	p)	[ $rflg ] && errflg=y
		pflg=y 
		;;
	r)	[ $pflg ] && errflg=y
		rflg=y 
		;;
	\?)	errflg=y
	esac
done

shift `expr $OPTIND - 1`

[ "$fflg" -o "$sflg" ] || errflg=y		# either -f xor -s
[ "$aflg" -a "$dflg" ] && errflg=y		# either -a xor -d
[ "$vflg" -a ! "$dflg" ] && errflg=y	# -d required if -v
[ \( -n "$rflg" -o -n "$pflg" \) -a ! "$aflg" ] && errflg=y
[ \( -n "$Aflg" -o -n "$Dflg" \) -a ! "$sflg" ] && errflg=y	# -s required id -A,-D
[ $# -ne 0 ] && errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

# Apps context
#
if [ $fflg ]
then
	ctxfile=`realpath $ctxfile`
else
	eval `ctx $option $voption ${Aflg:+-A$APPL_TOP} ${Dflg:+-D$DB_TOP} -f ${rflg:+-r} ${pflg:+-p} -s $s_systemname` || exit $?
	if [ $type = applications ]
	then
		ctxfile=$APPCTX
	else
		ctxfile=$DBCTX
	fi
fi
	
[ -r "$ctxfile" ] || { echo "$prog: cannot access $ctxfile" >&2; exit 1; }

if grep -q "Header: $header" $ctxfile
then
	:
else
	echo "$prog: $ctxfile is not a valid $type node context file" >&2
	exit 1
fi
echo $ctxfile

exit 0
