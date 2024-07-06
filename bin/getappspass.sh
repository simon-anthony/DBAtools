#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# getappspass: extract apps password
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
PATH=/bin:/usr/bin:BINDIR:SBINDIR export PATH
FPATH=BASEDIR/share/lib/functions export FPATH
unset ORACLE_HOME ORACLE_SID TWO_TASK

prog=`basename $0`
usage="[-f [-h <oracle_home>|-o]] -s <sid>"

ssh="ssh -o BatchMode=yes -o ChallengeResponseAuthentication=no"

while getopts s:fh:o opt 2>&-
do
	case $opt in
	s)	sid="$OPTARG"
		[ $aflg ] && errflg=y
		sflg=y
		;;
	f)	fflg=y	# force retrieval from database
		;;
	h)	hflg=y	# if no context file available
		[ $oflg ] && errflg=y
		oracle_home="$OPTARG"
		[ -d "$oracle_home" ] || {
			echo "$prog: no such directory '$oracle_home'" >&2; exit 1; }
		;;
	o)	oflg=y
		[ $hflg ] && errflg=y
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ ! "$sflg" ] && errflg=y
[ "$hflg" -a ! "$fflg" ] && errflg=y
[ "$oflg" -a ! "$fflg" ] && errflg=y
[ $# -ne 0 ] && errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

username=`id -un`

dbctxfile=`getctx -d -s $sid 2>&-`

# Simplest scenario is 11i - get password from wdbsvr.app
if appctxfile=`getctx -a -s $sid 2>&-`
then
	eval `ctxvar -ifv s_dbSid $appctxfile`
	eval `ctxvar -ifv s_weboh_oh $appctxfile`
	eval `ctxvar -ifv s_dbhost $appctxfile`
	eval `ctxvar -ifv s_appsuser $appctxfile`
	eval `ctxvar -ifv s_dbuser $appctxfile`

	if [ -r $s_weboh_oh/Apache/modplsql/cfg/wdbsvr.app ]
	then
		if passwd=`awk '
			/\[DAD_'$s_dbSid']/,/;/ {
				if ($1 == "username")
					username = tolower($3)
				if ($1 == "password")
					password = tolower($3)
			}
			END {
				if (length(password) == 0)
					exit 1
				print password
				exit 
			}' $s_weboh_oh/Apache/modplsql/cfg/wdbsvr.app`
		then
			if [ ! "$fflg" ]
			then
				if [ "$username" != "$s_appsuser" ]
				then
					echo "$prog: current user not owner of applications tier ($s_appsuser)" >&2
					exit 1
				fi
				echo $passwd
				exit 0
			else
				if [ ! "$dbctxfile" -a "$username" != "$s_dbuser" ]
				then
					echo "$prog: current user not owner of database tier ($s_dbuser)" >&2
					exit 1
				fi
			fi
		fi
	fi
fi

# R12 or not Apps tier - try connection as sysdba and get info from database
if [ "$dbctxfile" -o "$hflg" -o "$oflg" ]
then
	eval `ctxvar -ifv s_dbSid $dbctxfile 2>&-`  || s_dbSid=$sid

	if [ "$hflg" ]
	then
		s_db_oh=$oracle_home
	elif [ "$oflg" ]
	then
		s_db_oh=`oratab -p $s_dbSid | cut -f2 -d:`
		if [ -z "$s_db_oh" ]
		then
			echo "$prog: cannot find oratab entry for $s_dbSid" >&2
			exit 1
		fi
	else
		eval `ctxvar -ifv s_db_oh $dbctxfile`
	fi

	eval `ctxvar -ifv s_dbuser $dbctxfile 2>&-` || s_dbuser=`ORACLE_HOME=$s_db_oh dbid`

	if [ "$username" != "$s_dbuser" ]
	then
		echo "$prog: current user not owner of database tier ($s_dbuser)" >&2
		exit 1
	fi

	if ORACLE_HOME=$s_db_oh ORACLE_SID=$s_dbSid oiconnect "/ as sysdba"
	then
		:
	else
		echo "$prog: cannot connect to database as sysdba ($s_dbSid)" >&2
		exit 1
	fi
elif [ "$appctxfile" ]	# no wdbsvr.app
then
	if $ssh -q -n $s_dbhost exit
	then
		:
	else
		echo $prog: cannot connect to s_dbhost $s_dbhost, set up ssh login >&2
		exit 1
	fi
	$ssh -n $s_dbhost BINDIR/$prog ${fflg:+-f} -s $sid
	exit
else
	echo $prog: cannot determine applications or database contexts for $sid >&2
	exit 1
fi

if passwd=`ORACLE_PATH=DATADIR/lib/sql ORACLE_HOME=$s_db_oh ORACLE_SID=$s_dbSid \
	$s_db_oh/bin/sqlplus -s <<-!
	/ as sysdba
	whenever sqlerror exit failure
	whenever oserror exit failure

	set linesize 32767 pagesize 0
	set feedback off trimout on trimspool on termout off heading off

	variable passwd varchar2(32)

	@decrypt.plb
	@get_pwd.plb

	BEGIN
		:passwd := apps.get_pwd;
	END;
	/

	DROP FUNCTION apps.decrypt;
	DROP FUNCTION apps.get_pwd;

	print passwd
	exit success
	!`
then
	if [ "A$passwd" != "A" ]
	then
		echo $passwd
		exit 0
	fi
fi

exit 1
