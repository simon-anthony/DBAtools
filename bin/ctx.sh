#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# ctx: find CTX file for Oracle e-Business Suite environment
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
PATH=/bin:/usr/bin:SBINDIR:BINDIR export PATH
unset SHLIB_PATH LD_LIBRARY_PATH ORACLE_HOME CONETXT_FILE

prog=`basename $0`
 
name=`expr "$prog" : 'ad\(.*\)$' 2>&-`

usage="[-x] [-q] [-a[-r|-p]|-d|-i][-f] [-n] [-I <inst_top>] [-A <appl_top>] [-D <db_top>] [-v<n>] [-h <host>]... [-s <s_systemname>]"

while getopts xadifnI:A:D:v:s:h:rpq opt 2>&-
do
	case $opt in
	x)	xflg=y						# debug
		;;
	a)	aflg=y						# print only appsdb ctx if it exists
		[ "$dflg" -o "$iflg" ] && errflg=y
		;;
	d)	dflg=y						# print only db ctx if it exists
		[ "$aflg" -o "$iflg" ] && errflg=y
		;;
	I)	Iflg=y
		INST_TOP=$OPTARG
		[ -d "$INST_TOP" -a -x "$INST_TOP" -a -r "$INST_TOP" ] || {
			echo $prog: cannot access $INST_TOP >&2; exit 1; 
		}
		;;
	A)	Aflg=y
		APPL_TOP=$OPTARG
		[ -d "$APPL_TOP" -a -x "$APPL_TOP" -a -r "$APPL_TOP" ] || {
			echo $prog: cannot access $APPL_TOP >&2; exit 1; 
		}
		;;
	D)	Dflg=y
		DB_TOP="$OPTARG"
		[ -d "$DB_TOP" -a -x "$DB_TOP" -a -r "$DB_TOP" ] || {
			echo $prog: cannot access $DB_TOP >&2; exit 1; 
		}
		;;
	f)	fflg=y						# printf path to context file
		;;
	n)	nflg=y						# do not print in variable format
		;;
	q)	qflg=y						# do not print errors
		;;
	i)	iflg=y						# node info
		[ "$aflg" -o "$dflg" ] && errflg=y
		;;
	v)	vflg=y						# constrain version to be no higher than n
		expr "$OPTARG" : '[0-9]\{1,2\}$' >&- || errflg=y
		maxvers=$OPTARG
		;;
	s)	s_systemname=$OPTARG 
		sflg=y				
		;;
	h)	if [ "X$OPTARG" = "Xany" -o "X$extra_hosts" = "Xany" ]
		then
			extra_hosts="any"
		else
			extra_hosts="$extra_hosts $OPTARG"
		fi
		hflg=y						# add supplied host name to search list
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

[ $# -eq 0 ] || errflg=y
[ \( -n "$rflg" -o -n "$pflg" \) -a ! "$aflg" ] && errflg=y
 
[ $errflg ] && { echo usage: $prog $usage >&2; exit 2; }

[ $xflg ] && qflg=
: ${s_systemname:=$ORACLE_SID}
: ${s_systemname:=$TWO_TASK}

[ "$s_systemname" ] || { echo $prog: TWO_TASK or ORACLE_SID not set >&2; exit 2; }
 
# Find XML configuration file(s)
unset APPCTX DBCTX
hostname=`hostname`
hosts="`gethostnames`" # all the names by which I am known

# debug: print out verbose information so that user can identify problems
debug() {
	[ $xflg ] && echo "$prog: $*" >&2
}
error() {
	[ $qflg ] || echo "$prog: $*" >&2
}

# ckctx: check whether context file is correct type
ckctx() {
	typeset aflg= dflg= errflg= header=

	while getopts ad opt 2>&1
	do
		case $opt in
		d)	dflg=y
			header=adxdbctx
			[ $aflg ] && errflg=y
			;;
		a)	aflg=y
			header=adxmlctx
			[ $dflg ] && errflg=y
			;;
		\?)	errflg=y
		esac
	done
	shift `expr $OPTIND - 1`
	[ $# -ne 1 ] && errflg=y
	[ $errflg ] && {
		print -u2 "usage: ckctx {-a|-d} <context_file>"; exit 2; }
	grep -qs "Header: $header" ${1:-nosuchfile}
}

# get_top: wrapper for rwtop to cope with multiple paths
#	$1 - rw type  $2 - path
#
get_top() {
	case $1 in
	db|app*|inst) : ;;
	*)		echo 'usage: get_top db|app SID' >&2; return 2
	esac
	rwtop $1 $2 2>&- && return 0 || rwtop $1 $2 2>&1 | 
		while read name path
		do
			if [ "$name" = PATH: ]
			then
				case $1 in
				db)		[ -f $path/*/rdbms/lib/dumpsga.o ] && {
						debug "$0 $1 returns $path"
						echo $path; return 0; } ;;
				app*)	[ -r $path/admin/topfile.txt ] && {
						debug "$0 $1 returns $path"
						echo $path; return 0; } ;;
				inst)	[ -d $path/appl/admin ] && {
						debug "$0 $1 returns $path"
						echo $path; return 0; } ;;
				esac
			fi
		done
	return 1
}

# Can I access APPL_TOP?

if [ "X$INST_TOP" = "X" ] 
then
	if [ ! "$pflg" ]
	then
		rflg=y	# default if no environment set
	fi
fi
if [ "$INST_TOP" ] || INST_TOP=`get_top inst $s_systemname`
then
	debug "INST_TOP set as $INST_TOP"
	export INST_TOP
fi

if [ "$APPL_TOP" ] || APPL_TOP=`get_top appl $s_systemname`
then
	debug "APPL_TOP set as $APPL_TOP"
	export APPL_TOP

	if [ "$INST_TOP" ]
	then
		APPL_TOP=$INST_TOP/appl
		debug "APPL_TOP set from INST_TOP as $APPL_TOP"
	fi

	found=
	for host in $hosts
	do
		ctx=${s_systemname}_$host
		debug "Trying contextname $ctx"
		appctxfile=$APPL_TOP/admin/$ctx.xml
		debug "Searching for default apps CONTEXT_FILE $appctxfile..."
		if ckctx -a $appctxfile
		then
			found=y
			debug "Found apps CONTEXT_FILE $appctxfile"
			break
		fi
	done
	if [ ! "$found" ]
	then
		# maybe it's RAC or load balanced?
		debug "No apps CONTEXT_FILE found with default name/location, trying RAC/load balanced combinations..."
		for appctxfile in `ls $APPL_TOP/admin/*.xml 2>&-`
		do
			if ckctx -a $appctxfile
			then
				debug "Examining apps CONTEXT_FILE $appctxfile..."		
			else
				debug "Not apps CONTEXT_FILE $appctxfile..."		
				continue
			fi
			s_cp_twotask=`ctxvar s_cp_twotask $appctxfile`
			debug "s_cp_twotask is $s_cp_twotask"
			s_dbSid=`ctxvar s_dbSid $appctxfile`
			debug "s_dbSid is $s_dbSid"
			if [ "$s_cp_twotask" = "$s_systemname" ]
			then
				debug "s_cp_twotask matches s_systemname $s_systemname"
				ctx=`ctxvar s_contextname $appctxfile`
				debug "Setting contextname $ctx"
				found=y
				break
			fi
		done
	fi
	
	if [ "$found" ]
	then
		debug "Applications contextname found and set to $ctx"
		APPCTX=$ctx 
		s_com=`ctxvar s_com $appctxfile` 
		s_cp_twotask=`ctxvar s_cp_twotask $appctxfile` 

		if [ ! "$sflg" ]
		then 
			s_dbSid=`ctxvar s_dbSid $appctxfile`
			debug "s_dbSid set to $s_dbSid"
			s_tools_twotask=`ctxvar s_tools_twotask $appctxfile`
			debug "s_tools_twotask set to $s_tools_twotask"
		fi
	elif [ "$dflg" ]	# if database node ignore if missing
	then
		:
		debug "No applications context file found, ignoring as db node requested"
		unset appctxfile APPCTX ctx
	else
		if [ $aflg ]
		then
			error "cannot determine XML context file location [APPS] for $s_systemname" 
			exit 1
		fi
		unset appctxfile APPCTX ctx
	fi
fi 
if [ "X$appctxfile" != "X" ]
then
	for var in s_file_edition_type s_current_base s_other_base
	do
		eval `ctxvar -ifv $var $appctxfile`
	done

	if [ $rflg ]
	then
		if [ "$s_file_edition_type" != "run" ]
		then 
			debug "Swapping to 'run' filesystem: $s_other_base"
			debug "other base: $s_other_base"
			debug "current base: $s_current_base"
			appctxfile=$s_other_base${appctxfile#$s_current_base}
		fi
	elif [ $pflg ]
	then
		if [ "$s_file_edition_type" != "patch" ]
		then 
			debug "Swapping to 'patch' filesystem: $s_other_base"
			appctxfile=$s_other_base${appctxfile#$s_current_base}
		fi
	fi
fi

if [ "$DB_TOP" ] || DB_TOP=`get_top db $s_systemname`
then
	if oracle_homes=`ls $DB_TOP/*/bin/oracle 2>&-`
	then
		:
	else
		debug "No database server ORACLE_HOMEs found"
		if [ -z "$APPCTX" ]
		then
			error "cannot determine any XML context file location for $s_systemname"
			exit 1
		fi
	fi
	# Find highest version ORACLE_HOME installed
	#
	oracle_homes=`for path in $oracle_homes
		do
			oh=\`echo $path | sed 's;\(.*\)/bin/oracle;\1;'\`
			if LD_LIBRARY_PATH=$oh/lib:$oh/rdbms/lib ckres -n -q $path
			then
				:
			else
				debug "Unresolved libraries in $path, skipping ORACLE_HOME"
				continue
			fi
			vers=\`ORACLE_HOME=$oh SHLIB_PATH=$oh/lib LIBPATH=$oh/lib dbvers -3\`
			debug "Found ORACLE_HOME: $oh, version $vers"
			case $vers in
			9.*|10.*|11.*|12.*)	
				if [ $vflg ] 
				then
					[ ${vers%%.*} -gt $maxvers ] && continue
				fi
				debug "Candidiate server ORACLE_HOME: $oh"
				;;
			*)	continue	# ignore 8.0.6/iAS
			esac
			echo $vers $oh
		done | sort -n -r -t. -k 1,1 -k 2,2 -k 3,3 | sed -n '/OLD/d; s;[^ ]* \([^ ]*\);\1;p;'`

	found=
	if [ "$oracle_homes" ]
	then
		for oracle_home in $oracle_homes
		do
			debug "Searching server ORACLE_HOME: $oracle_home..."
			if [ $ctx ]
			then
				debug "Searching for default db CONTEXT_FILE $oracle_home/appsutil/$ctx.xml..."
				for host in $hosts
				do
					if ckctx -d $oracle_home/appsutil/$ctx.xml 
					then
						dbctxfile=$oracle_home/appsutil/$ctx.xml
						debug "Found db CONTEXT_FILE $dbctxfile, verifying contents..."
						if [ \( "`ctxvar s_dbSid $dbctxfile`" = $s_systemname -o \
							 "`ctxvar s_dbGlnam $dbctxfile`" = $s_systemname -o \
							 "`ctxvar s_dbService $dbctxfile`"  = $s_systemname \) -a \
							 "`ctxvar s_hostname $dbctxfile`"  = $host ]
						then
							debug "Contents of CONTEXT_FILE $dbctxfile match $s_systemname"
							found=y
							break
						fi
					fi
				done
			fi
			if [ ! "$found" ]
			then
				debug "No default db CONTEXT_FILE in $oracle_home/appsutil/ found, checking for other names..."
				for host in $hosts $extra_hosts
				do
					[ "$host" = "any" ] && continue
					for i in $s_systemname $s_tools_twotask $s_cp_twotask
					do
						ctx=${i}_$host
						dbctxfile=$oracle_home/appsutil/$ctx.xml
						debug "Searching for db CONTEXT_FILE $dbctxfile..."
						if ckctx -d $dbctxfile
						then 
							debug "Found db CONTEXT_FILE $dbctxfile, verifying contents..."
							if [ \( "`ctxvar s_dbSid $dbctxfile`" = $s_systemname -o \
								 "`ctxvar s_dbGlnam $dbctxfile`" = $s_systemname -o \
								 "`ctxvar s_dbService $dbctxfile`"  = $s_systemname \) -a \
								 "`ctxvar s_hostname $dbctxfile`" = $host ]
							then
								debug "Contents of CONTEXT_FILE $dbctxfile match $s_systemname"
								found=y
								break
							fi
						fi
					done
					[ "$found" ] && break
				done
				if [ ! "$found" ]	# try inspecting the files - could be RAC
				then
					debug "No db CONTEXT_FILE found with default name/location, trying RAC/load balanced combinations..."
					for dbctxfile in `ls $oracle_home/appsutil/*.xml 2>&-`
					do
						for host in $hosts
						do
							debug "Verifying contents of db CONTEXT_FILE $dbctxfile..."
							if ckctx -d $dbctxfile && [ \( "`ctxvar s_dbSid $dbctxfile`" = $s_systemname -o \
								 "`ctxvar s_dbGlnam $dbctxfile`" = $s_systemname -o \
								 "`ctxvar s_dbService $dbctxfile`"  = $s_systemname \) -a \
									 "`ctxvar s_hostname $dbctxfile`" = $host ]
							then
								debug "Contents of CONTEXT_FILE $dbctxfile match $s_systemname"
								ctx=`ctxvar s_contextname $dbctxfile`
								debug "Database contextname found and set to $ctx"
								found=y 
								break
							fi
						done
						[ "$found" ] && break
					done
				fi
				if [ ! "$found" -a "X$extra_hosts" = "Xany" ]	# get first file with sid and any hostname (for cloning use)
				then
					debug "No db CONTEXT_FILE found for RAC, trying any hostname..."
					for i in $s_dbSid $s_tools_twotask $s_cp_twotask
					do
						for dbctxfile in `ls $oracle_home/appsutil/${i}_*.xml 2>&-`
						do
							debug "Verifying contents of db CONTEXT_FILE $dbctxfile..."
							if ckctx -d $dbctxfile && [ \( "`ctxvar s_dbSid $dbctxfile`" = $s_systemname -o \
								 "`ctxvar s_dbGlnam $dbctxfile`" = $s_systemname -o \
								 "`ctxvar s_dbService $dbctxfile`"  = $s_systemname \) ]
							then
								debug "Contents of CONTEXT_FILE $dbctxfile match $s_systemname"
								ctx=`ctxvar s_contextname $dbctxfile`
								debug "Database contextname found and set to $ctx"
								found=y 
								break
							fi
							[ "$found" ] && break
						done
					done
				fi
				[ "$found" ] && break || unset dbctxfile
			fi
		done

		if [ ! "$found" ]
		then
			if [ $dflg ]
			then
				error "cannot determine XML context file location [DATABASE] for $s_systemname" 
				exit 1
			fi
			unset dbctxfile DBCTX ctx
		fi

		[ $ORACLE_HOME ] && ORACLE_HOME=$oracle_home export ORACLE_HOME
		[ $ctx ] && DBCTX=$ctx export DBCTX
	fi
else
	if [ -z "$APPCTX" ]
	then
		error "cannot determine any XML context file location for $s_systemname"
		exit 1
	fi
fi
[ $APPCTX ] && export APPCTX

if [ $fflg ]
then
	if [ "$aflg" ]
	then
		[ ! "$appctxfile" ] && exit 1
		[ $nflg ] && echo $appctxfile || echo APPCTX=$appctxfile 
	elif [ "$dflg" ]
	then
		[ ! "$dbctxfile" ] && exit 1
		[ $nflg ] && echo $dbctxfile  || echo DBCTX=$dbctxfile 
	else
		[ $nflg ] && echo $appctxfile || echo APPCTX=$appctxfile
		[ $nflg ] && echo $dbctxfile  || echo DBCTX=$dbctxfile
	fi
else
	if [ "$aflg" ]
	then
		[ ! "$APPCTX" ] && exit 1
		[ $nflg ] && echo $APPCTX || echo APPCTX=$APPCTX
	elif [ "$dflg" ]
	then
		[ ! "$DBCTX" ] && exit 1
		[ $nflg ] && echo $DBCTX  || echo DBCTX=$DBCTX
	else
		[ $nflg ] && echo $APPCTX || echo APPCTX=$APPCTX
		[ $nflg ] && echo $DBCTX  || echo DBCTX=$DBCTX
	fi
fi

if [ ! "$iflg" ]
then
	exit 0
fi

# Check whether a web/forms tier, cp tier, cp + DB tier, or DB only tier

exec 2>/dev/null
s_isForms=`ctxvar -if s_isForms $appctxfile` ||
	s_isForms=`ctxvar -if s_isForms $dbctxfile`
[ $dbctxfile ] && s_isDB=`ctxvar -if s_isDB $dbctxfile` ||
	s_isDB=`ctxvar -if s_isDB $appctxfile`
s_isAdmin=`ctxvar -if s_isAdmin $appctxfile` ||
	s_isAdmin=`ctxvar -if s_isAdmin $dbctxfile`
s_isWeb=`ctxvar -if s_isWeb $appctxfile` ||
	s_isWeb=`ctxvar -if s_isWeb $dbctxfile`
s_isConc=`ctxvar -if s_isConc $appctxfile` ||
	s_isConc=`ctxvar -if s_isConc $dbctxfile`

printf "DB=%-.1s Conc=%-.1s Admin=%-.1s Web=%-.1s Forms=%-.1s\n" \
$s_isDB $s_isConc $s_isAdmin $s_isWeb $s_isForms

exit 0
