#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#  dbh: inspect database header files
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
################################################################################

PATH=/usr/bin:BINDIR export PATH

usage="[-b <blocksize>] [-o ORACLE_HOME] file..."
prog=`basename $0`
errflag=
 
while getopts s:b:o: opt 2>&-
do
	case $opt in
	s)	sflg=y				# SID to check against
		testsid=$OPTARG
		;;
	b)	bflg=y
		blocksize=$OPTARG
		;;
	o)	oflg=y
		ORACLE_HOME=$OPTARG
		export ORACLE_HOME
		if [ `uname -s` = HP-UX ]
		then
			SHLIB_PATH=$ORACLE_HOME/lib export SHLIB_PATH
		else
			LD_LIBRARY_PATH=$ORACLE_HOME/lib export LD_LIBRARY_PATH
		fi
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`
 
[ $# -ge 1 ] || errflg=y

[ $errflg ] && { echo usage: $prog $usage >&2; exit 2; }

errors=0

# To handle different blocksizes we need to use the version which
# runs bbed. However, a valid ORACLE_HOME (possibly with appropriate
# SHLIB_PATH, LD_LIBRARY_PATH settings) will need to be set.
# Also sbin/dbh requires absolute paths.

realpath2() {
	if expr "$1" : '.*/.*' >/dev/null 2>&1	# has directory component
	then
		dir=`dirname $1`
		( cd $dir 2>&- && echo `pwd`/`basename $1` )
	else									# just filename
		echo `pwd`/$i
	fi
}

realpaths() {
	for i
	do
		realpath2 $i
	done
}

if [ $bflg ]
then
	exec SBINDIR/dbh -b $blocksize `realpaths $*`
fi

for file 
do
	[ -r $file ] || {
		echo $prog: cannot read \'$file\' >&2
		errors=`expr $errors + 1`
		continue
	}
	# Redo Log?
	# 0002034   D   I   N   O  \0  \0  \0  \0  \0   v 360 267  \0 003 350  \0
	offset=0002034
	sid=`od -c $file +$offset 2>&- |
		 sed -n '
			/[^\\]\{1,\}.*/ {
				s%[0-9]\{1,\} *\([^\\]\{1,\}\).*%\1%;
				s% %%g; p; q;
			}
			q;'`
	if [ "$sid" ]
	then
		[ -n "$sflg" -a "$sid" != "$testsid" ] && {
			errors=`expr $errors + 1`
			sid="$sid - unexpected"
		}
		echo $prog: $file redo log [$sid]
		continue
	fi

	# Database File?
	# 0020040   P   E   R   F  \0  \0  \0  \0  \0 013 360 325  \0   > 177 200
	offset=0020040
	sid=`od -c $file +$offset 2>&- |
		 sed -n '
			/[^\\]\{1,\}.*/ {
				s%[0-9]\{1,\} *\([^\\]\{1,\}\).*%\1%;
				s% %%g; p; q;
			}
			q;'`
	if [ "$sid" ]
	then
		[ -n "$sflg" -a "$sid" != "$testsid" ] && {
			errors=`expr $errors + 1`
			sid="$sid -unexpected"
		}
		echo $prog: $file database file [$sid]
		continue
	fi

	# Control File?
	# 0040040   P   E   R   F  \0  \0  \0  \0  \0  \0 220 003  \0  \0 007 034
	offset=0040040
	sid=`od -c $file +$offset 2>&- |
		 sed -n '
			/[^\\]\{1,\}.*/ {
				s%[0-9]\{1,\} *\([^\\]\{1,\}\).*%\1%;
				s% %%g; p; q;
			}
			q;'`
	if [ "$sid" ]
	then
		[ -n "$sflg" -a "$sid" != "$testsid" ] && {
			errors=`expr $errors + 1`
			sid="$sid -unexpected"
		}
		echo $prog: $file control file [$sid]
		continue
	fi

	echo $prog: $file unknown type
	errors=`expr $errors + 1`
done

exit $errors
