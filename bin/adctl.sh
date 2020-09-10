#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#
# Shortcut to run AD Scripts individually
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
unset ORACLE_HOME ORACLE_SID TWO_TASK TNS_ADMIN SQL_PATH ORACLE_PATH

prog=`basename $0`
usage() {
	cat <<!
usage: $prog -s <s_systemname> [-p|-c|-r] [-n] [-f] <script> [<args>...]
       $prog -s <s_systemname> -l [-l|-e|-u]
!
[ $help ] && cat <<!

   -s <s_systemname>          Use <s_systemname> to determine context

   -p                Append apps/password to <args>...

   -c                Append operator/password to <args>...

   -r                Non-blocking read for username and password

   -n                Print script and arguments rather then execute

   -f                Force scripts whose status is "disabled" in the context
                     file to be executed

   -l                List scripts - repeat for long listing

   -e                Show enabled status from context file

   -u                Show usage message from script

!
}

while getopts ns:lpfercu opt 2>&-
do
	case $opt in
	s)	s_systemname="$OPTARG"
		sflg=y
		;;
	l)	[ $llflg ] && errflg=y
		[ $lflg ] && llflg=y
		lflg=y
		[ "$uflg" -a "$llflg" ] && errflg=y
		[ "$eflg" -a "$llflg" ] && errflg=y
		;;
	f)	fflg=y
		;;
	n)	nflg=y
		run=echo
		;;
	p)	pflg=y
		[ "$rflg" -o "$cflg" ] && errflg=y
		;;
	c)	cflg=y
		[ "$rflg" -o "$pflg" ] && errflg=y
		;;
	r)	rflg=y
		[ "$pflg" -o "$cflg" ] && errflg=y
		;;
	e)	eflg=y
		[ "$uflg" -o "$llflg" ] && errflg=y
		;;
	u)	uflg=y
		[ "$eflg" -o "$llflg" ] && errflg=y
		;;
	\?)	errflg=y
		help=y
	esac
done
shift `expr $OPTIND - 1`

[ $sflg ] || errflg=y
[ $# -eq 0 -a ! "$lflg" ] && errflg=y

[ "$eflg" -a ! "$lflg" ] && errflg=y	# -l mandatory if -e
[ "$uflg" -a ! "$lflg" ] && errflg=y	# -l mandatory if -u

[ $errflg ] && { usage >&2; exit 2; }

if [ ! "$lflg" ]
then
	pat=$1
	shift
	args="$@"
fi

# Context file(s)
#
if appctxfile=`getctx -a -s $s_systemname` 
then
	:
else
	print -u2 "$prog: cannot find applications context file"
	exit 1
fi

if [ $pflg ]
then
	if password=`getappspass -s $s_systemname`
	then
		args="$args apps/$password"
	else
		print -u2 "$prog: unable to obtain APPS password"
		exit 1
	fi
elif [ $cflg ]
then
	if unpw=`adcpusr -s $s_systemname`
	then
		args="$args $unpw"
	else
		print -u2 "$prog: unable to obtain CPUSER password"
		exit 1
	fi
elif [ $rflg ]
then
	username=`nbread`
	password=`nbread`

fi

################################################################################

# is_enabled:
#	$1 the script name is checked to determine whether or not it is enabled
is_enabled() {
	typeset script
	[ $# -eq 1 ] || { print -u2 "is_enabled: no argument supplied"; return 1; }
	script=$1
	[ $fflg ] && return 0
	[ X"$script" = X`adstatus -e -s $s_systemname | awk '$(NF) == "'$script'" { print $(NF) }'` ]
}

# lsscripts:
#   print the names and pathnames of all control scripts
lsscripts() {
	grep '<ctrl_script' $appctxfile | ctxvar -i -p ^s_.\{1,\}ctrl\$
}

if [ $lflg ]
then
	if [ $eflg ]
	then
		adstatus -e -s $s_systemname
	else
		lsscripts | 
		while read line
		do
			var=${line%=*}
			if [ $var != ${var%-*} ] # variable names cannot contain '-'
			then
				var=`echo $var | sed 's;-;X;g'`
			fi
				
			path=${line#*=}
			eval $var=$path
			file=`basename $path`
			[ -f $path ] && sfx= || sfx=!
			found=	# do not show duplicates, but do not sort output
			for i in $list
			do
				if [ $i = $file$sfx ]
				then
					found=y
					break
				fi
			done
			[ $found ] || list="$list $file$sfx"
			[ $found ] || listv="$listv $var"
		done
		if [ $llflg ]
		then
			for var in $listv
			do
				path=`eval echo \\${$var}`
				if [ -f $path ]
				then
					ls -l $path | awk '{ printf("%-70s %8s %8s %-10s\n", $(NF), $3, $4, $1); }'
				else
					printf "%-70s does not exist\n" $path
				fi
			done
		elif [ $uflg ]
		then
			for var in $listv
			do
				path=`eval echo \\${$var}`
				if [ -f $path ]
				then
					sed -n '/USAGE/ { n; s/# | *//; p; q; }' $path
				else
					printf "# %s does not exist\n" `basename $path`
				fi
			done
		else
			for file in $list
			do
				echo $file
			done
		fi
	fi
	exit 0
fi

found=0 ad=
for file in `lsscripts | xargs -n1 basename`
do
	if [ "X$file" = "X$pat" -o "X$file" = "Xad${pat}ctl" -o "X$file" = "Xad${pat}ctl.sh" ]
	then
		if [ $found -gt 0 ]
		then
			if [ "$file" = "$script" ]
			then
				: # it's OK, already have an enabled version of this script
			else
				(( found += 1 ))
			fi
			continue
		fi
		(( found += 1 ))
		script=$file ad=y
	elif length=`expr "$file" : ".*$pat.*"`
	then
		if [ $found -ge 1 -a "$ad" ]
		then
			: # already matched an 'ad' script
		else
			if [ $found -gt 0 ]
			then
				if [ "$file" = "$script" ]
				then
					: # it's OK, already have an enabled version of this script
				else
					(( found += 1 ))
				fi
				continue
			fi
			(( found += 1 ))
			script=$file
		fi
	fi
done

if [ $found -eq 0 ]
then
	print -u2 "$prog: no script matched by '$pat'"
	exit 1
elif [ $found -gt 1 ]
then
	print -u2 "$prog: '$pat' too many matches ($found)"
	exit 1
fi

scriptpath=`lsscripts | sed -n "/\/$script\$/ { s;.*=\(.*/$script\)\$;\1;p; q; }"`

if [ ! "$fflg" ]
then
	if is_enabled $script
	then
		:
	else
		print -u2 "$prog: $script is disabled, use -f"
		exit 0
	fi
fi
if [ ! -f $scriptpath ]
then
	print -u2 "$prog: $script does not exist [$scriptpath]"
	exit 1
fi
if [ $nflg ]
then
	if [ $rflg ]
	then
		echo "{ echo ${username:-apps}; echo $password; } | $scriptpath $args"
	else
		echo $scriptpath $args
	fi
	exit
fi 
if [ $rflg ]
then
	{ echo ${username:-apps}; echo $password; } | $scriptpath $args
	exit 
fi
exec $scriptpath $args
