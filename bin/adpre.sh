#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#
# Shortcut to run adpreclone
# Note: need zip in PATH because this is checked before .env file is sourced.
#
################################################################################
PATH=/usr/local/bin:/bin:/usr/bin:BINDIR:/opt/perl/bin export PATH
FPATH=BASEDIR/share/lib/functions export FPATH

prog=`basename $0`
usage="[-y] [-da] [-p password] -s <s_systemname>"

while getopts ads:p:y opt 2>&-
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
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $sflg ] || errflg=y
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
	if [ $pflg ]
	then
		:	# password supplied
	elif password=`getappspass -s $s_systemname`
	then
		:
	else
		echo "$prog: supply a password with -p" >&2; exit 2
	fi
fi

if [ ! "$aflg" -a ! "$dflg" ]
then
	appctxfile=`getctx -a -s $s_systemname`
	dbctxfile=`getctx -d -s $s_systemname` 
fi

# Zip version - must be between 2.3 and 3.0
zipfound=
if whence zip >&-
then
	version=`zip -h | awk '$(NF) == "Usage:" { print $2 }'`
	release=${version%.*} level=${version#*.}
	case  "$release" in
	2)	[ "$level" -gt 3 ] && zipfound=y ;;
	3)	[ "$level" -eq 0 ] && zipfound=y ;;
	esac
fi
################################################################################

if [ $dbctxfile ]
then
	s_dbSid=`ctxvar -if s_dbSid $dbctxfile`	# ORACLE_SID
	s_db_local=`ctxvar -if s_db_local $dbctxfile`
	s_dbGlnam=`ctxvar -if s_dbGlnam $dbctxfile`
	db_ctrl_script=`ctxvar -if s_dlsnctrl $dbctxfile`
	db_scripts=`dirname $db_ctrl_script`
	s_db_oh=`ctxvar -if s_db_oh $dbctxfile`
	[ ! "$zipfound" ] && PATH=$s_db_oh/bin:$PATH
fi

if [ $appctxfile ]
then
	s_dbSid=`ctxvar -if s_dbSid $appctxfile`	# ORACLE_SID
	s_tools_twotask=`ctxvar -if s_tools_twotask $appctxfile`
	s_dbGlnam=`ctxvar -if s_dbGlnam $appctxfile`
	apps_ctrl_script=`ctxvar -if s_tnsctrl $appctxfile`
	apps_scripts=`dirname $apps_ctrl_script`
	s_tools_oh=`ctxvar -if s_tools_oh $appctxfile`
	s_tools_tnsadmin=`ctxvar -if s_tools_tnsadmin $appctxfile`
	[ ! "$zipfound" ] && PATH=$s_tools_oh/bin:$PATH
fi

if [ $s_dbSid ]
then
	if [ $dbctxfile ]
	then
		if [ -d ${db_scripts:-/nosuchdir} ]
		then
			:
		else
			echo AutoConfig not yet implemented on DB Tier >&2
			exit 1
		fi
		echo cd $db_scripts; echo perl adpreclone.pl dbTier
	fi
	if [ $appctxfile ]
	then
		echo cd $apps_scripts; echo perl adpreclone.pl appsTier
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

if [ $dbctxfile ]
then
	if cd $db_scripts 
	then
		if [ "A$password" != "A" ]
		then
			perl adpreclone.pl dbTier <<-!
				$password
			!
		else
			perl adpreclone.pl dbTier 
		fi
	fi
fi
[ $appctxfile ] && cd $apps_scripts && perl adpreclone.pl appsTier

exit 0
