#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#
# Shortcut to run AutoConfig
#
################################################################################
PATH=/usr/local/bin:/bin:/usr/bin:BINDIR export PATH
FPATH=BASEDIR/share/lib/functions export FPATH

prog=`basename $0`
usage="[-y] [-v[-q]] [-da] [-p password] -s <s_systemname>"

while getopts ads:p:yvq opt 2>&-
do
	case $opt in
	a)	aflg=y
		;;
	d)	dflg=y
		;;
	s)	s_systemname="$OPTARG"
		sflg=y
		;;
	p)	password="$OPTARG"
		pflg=y
		;;
	y)	yflg=y
		;;
	v)	vflg=y
		;;
	q)	qflg=y
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $sflg ] || errflg=y
[ "$qflg" -a ! "$vflg" ] && errflg=y	# require -v if -q
[ $# -ne 0 ] && errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

# Context file(s)
#
if [ $aflg ]
then
	appctxfile=`getctx -a -s $s_systemname` || exit $?
fi

if [ $dflg ]
then
	dbctxfile=`getctx -d -s $s_systemname` || exit $?
fi

if [ $pflg ]
then
	:	# password supplied
elif password=`getappspass -s $s_systemname`
then
	:
else
	echo "$prog: supply a password with -p" >&2; exit 2
fi

if [ ! "$aflg" -a ! "$dflg" ]
then
	appctxfile=`getctx -a -s $s_systemname`
	dbctxfile=`getctx -d -s $s_systemname` 
fi

################################################################################

if [ $dbctxfile ]
then
	s_dbSid=`ctxvar -if s_dbSid $dbctxfile`	# ORACLE_SID
	s_db_local=`ctxvar -if s_db_local $dbctxfile`
	s_dbGlnam=`ctxvar -if s_dbGlnam $dbctxfile`
	db_ctrl_script=`ctxvar -if s_dlsnctrl $dbctxfile`
	db_acfg=`dirname $db_ctrl_script`/adautocfg.sh 
	s_db_oh=`ctxvar -if s_db_oh $dbctxfile`
fi
if [ $appctxfile ]
then
	s_dbSid=`ctxvar -if s_dbSid $appctxfile`	# ORACLE_SID
	s_tools_twotask=`ctxvar -if s_tools_twotask $appctxfile`
	s_dbGlnam=`ctxvar -if s_dbGlnam $appctxfile`
	apps_ctrl_script=`ctxvar -if s_tnsctrl $appctxfile`
	apps_acfg=`dirname $apps_ctrl_script`/adautocfg.sh 
	s_tools_oh=`ctxvar -if s_tools_oh $appctxfile`
	s_tools_tnsadmin=`ctxvar -if s_tools_tnsadmin $appctxfile`
fi

if [ $s_dbSid ]
then
	[ $pflg ] || password=`getappspass -s $s_systemname`

	if [ $dbctxfile ]
	then
		if [ -r $db_acfg ]
		then
			:
		else
			# AutoConfig not yet implemented on DB Tier
			cd $s_db_oh/appsutil/bin
			db_acfg="$s_db_oh/appsutil/bin/adconfig.sh contextfile=$dbctxfile"
		fi
		echo $db_acfg appspass=$password
		(
		if ORACLE_HOME=$s_db_oh ORACLE_SID=$s_db_local oiconnect apps/$password
		then
			exit 0
		else
			echo $prog: ERROR - user apps cannot connect - check appspass >&2
			echo Check listener is up >&2
			exit 1
		fi ) || exit 1
	fi
	if [ $appctxfile ]
	then
		echo $apps_acfg appspass=$password
		(
		if ORACLE_HOME=$s_tools_oh TNS_ADMIN=$s_tools_tnsadmin \
			ORACLE_SID= TWO_TASK=$s_tools_twotask oiconnect apps/$password
		then
			exit 0
		else
			echo $prog: ERROR - user apps cannot connect - check appspass >&2
			echo Check connection with SQL\*Plus, if ORA-12505 it may be that TNS configuration for RAC has not yet been generated. >&2
			exit 1
		fi ) || exit 1
	fi
fi
if [ ! "$yflg" ]
then
	ans=`ckyorn -d y -p "Continue?"` || exit $?
	case $ans in
	y) : ;;
	*) exit
	esac
fi

unset PERL5LIB
for cmd in "$db_acfg" "$apps_acfg"
do
	[ "X$cmd" = "X" ] && continue
	if [ $vflg ]
	then
		success=
		$cmd appspass=$password |&
		while read -p line
		do
			if path=`expr "$line" : 'The log .* located at: \([^ ]\{1,\}\)$'`
			then
				log=$path
				[ $qflg ] && print "$prog: log is $log"
				[ $qflg ] && \
					print -n "$prog: waiting for autoconfig to complete... "
			elif expr "$line" : 'AutoConfig completed successfully\.' >&-
			then
				success=y
			fi
			[ $qflg ] || echo "$line"
		done
			
		# check log file first
		if [ ! "$log" ]
		then
			[ $qflg ] && \
				print -n "$prog: waiting for adclone to complete... failed"
			[ $qflg ] && print -u2 "$prog: error: failed to execute autoconfig"
			exit 1
		elif [ -r "$log" ]
		then
			if grep -qs "AutoConfig is exiting with status 0" $log
			then
				:
			else
				[ $qflg ] && print "failed"; exit 1
			fi
		fi

		# then printed message
		if [ $success ]
		then
			[ $qflg ] && print "done"
		else
			[ $qflg ] && print "failed"; exit 1
		fi
	else
		$cmd appspass=$password || exit $?
	fi
done

exit 0
