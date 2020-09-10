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
PATH=/usr/bin:BINDIR export PATH

prog=`basename $0`

usage() {
	cat <<!
usage: $prog -s <sid> [-h <oracle_home>|-o] [-d <datetime>] [-e <n>] [-a <n>]...
!
[ $help ] && cat <<!

   -h <oracle_home>
            set the ORACLE_HOME directly as <oracle_home>

   -s <sid> use <sid> to determine the ORACLE_HOME

   -o       if using -s, use oratab rather than the database context file

   -d <datetime>
            specify the date and time in the format "%Y-%m-%d %R"

   -e <n>   problem code to search for, default 600

   -a <n>   optional arguments for problem, can be specified multiple times

!
}

ecode=600

while getopts s:h:od:a:e: opt 2>&-
do
	case $opt in
	s)	sid="$OPTARG"
		lsid=`echo $sid | tr '[:upper:]' '[:lower:]'`
		sflg=y
		;;
	h)	ORACLE_HOME="$OPTARG"
		export ORACLE_HOME
		hflg=y
		[ $oflg ] && errflg=y
		;;
	o)	oflg=y				# use oratab entry
		[ $hflg ] && errflg=y
		;;
	d)	dflg=y
		if expr "$OPTARG" : '20[0-9][0-9]-[0-9][0-9]-[0-9][0-9] [0-9][0-9]:[0-9][0-9]$' >&-
		then
			datetime="$OPTARG"
		else
			echo $prog: bad date time specification >&2
			exit 2
		fi
		;;
	e)	eflg=y
		if expr "$OPTARG" : '[0-9]\{1,\}$' >&-
		then
			ecode="$OPTARG"
		else
			echo $prog invalid error number >&2
			exit 2
		fi
		;;
	a)	if expr "$OPTARG" : '.*,' >&-
		then
			for arg in `echo "$OPTARG" | sed 's;,; ;g'`
			do
				args[${aflg:=0}]="$arg" # comma separated list
				(( aflg += 1 ))
			done
		else
			args[${aflg:=0}]="$OPTARG" # args for problem
			(( aflg += 1 ))
		fi
		;;
	\?)	errflg=y
		help=y
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 0 ] || errflg=y
[ "$sflg" ] || errflg=y					# -s required
[ "$oflg" -a ! "$sflg" ] && errflg=y	# -s required for -o
[ ! "$sflg" -a ! "$hflg" ] && errflg=y	# at least one of -h or -s required

[ $errflg ] && { usage >&2; exit 2; }

if [ $oflg ]
then
	oratab=`ls /var/opt/oracle/oratab /etc/oratab 2>&-`
	[ $oratab ] || { echo "$prog: cannot find oratab" >&2; exit 1; }
	ORACLE_HOME=`awk -F: '$1 == "'$sid'" { print $2 }' $oratab`
	[ $ORACLE_HOME ] || {
		echo "$prog: cannot find '$sid' in $oratab" >&2; exit 1; }
	ORACLE_SID=$sid export $ORACLE_SID
elif [ $hflg ]
then
	ORACLE_SID=$sid export ORACLE_SID
else
	if dbctxfile=`getctx -d -s $sid` 
	then
		eval `ctxvar -ifv s_db_home_file $dbctxfile 2>&-`
		if [ ! -r "$s_db_home_file" ]
		then
			ORACLE_HOME=`ctxvar s_db_oh $dbctxfile`
			ORACLE_SID=`ctxvar s_dbSid $dbctxfile`
			export ORACLE_HOME ORACLE_SID
		else
			. $s_db_home_file
		fi
		#$src || exit $?
	fi
fi

# build predicates list

if [ $dflg ]
then
	predicates="LASTINC_TIME >= '$datetime'"
fi

if [ $aflg ]
then
	i=0
	if [ $dflg ]
	then
		con=AND
		[ ${#args[*]} -gt 1 ] && con="$con ("
	fi
	while [ $i -lt ${#args[*]} ]
	do
		predicates="$predicates $con PROBLEM_KEY LIKE 'ORA $ecode [${args[$i]}]%'"
		con=OR
		(( i += 1 ))
	done
	[ ${#args[*]} -gt 1 -a -n "$dflg" ] && predicates="$predicates )"
else
	[ -n "$predicates" ] \
		&& predicates="$predicates AND PROBLEM_KEY LIKE 'ORA $ecode%'" \
		|| predicates="PROBLEM_KEY LIKE 'ORA $ecode%'" 
fi

PATH=$ORACLE_HOME/bin:$PATH 
homepaths=`adrci <<-! | sed -n '/^diag\/rdbms\// p'
	SHOW HOMEPATH
!`

if [ -z "$homepaths" ]
then
	adr_base=`adrci <<-! | sed -n 's;^ADR base = "\(.*\)";\1;p'
	!`
	if [ -z "$adr_base" ]
	then
		echo $prog: cannot determine ADR Base >&2
		exit 1
	fi
	cd $adr_base
	homepath=`ls -d diag/rdbms/*/$ORACLE_SID 2>&-` 
else
	for homepath in $homepaths
	do
		if expr "$homepath" : "diag/rdbms/$lsid*/$ORACLE_SID" >&-
		then
			break
		else
			unset homepath
		fi
	done
fi
if [ -z "$homepath" ]
then
	echo $prog: cannot determine HOMEPATH >&2
	exit 1
fi

tmpfile=`mktemp`

adrci <<-! | sed -n '/^\*\*/,/^[0-9]\{1,\} rows fetched/ { /^\*\*/ d; /rows fetched/ d; p; }' | tee -a $tmpfile
	SET HOMEPATH $homepath
	SHOW PROBLEM -p "$predicates"
!
if [ -s $tmpfile ]
then
	err=0
fi

rm -f $tmpfile
exit ${err:-1}
