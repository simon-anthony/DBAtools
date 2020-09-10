#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# adccpusr: set/get conccurrent manager operator and password: 403537.1
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
PATH=/bin:/usr/bin:BINDIR:SBINDIR:/usr/lbin export PATH
FPATH=BASEDIR/share/lib/functions export FPATH
unset ORACLE_HOME ORACLE_SID TWO_TASK TNS_ADMIN SQL_PATH ORACLE_PATH

prog=`basename $0`
usage="[-d] -s <s_systemname> [-p <apps_password> [-c <cp_user>]] [-m]"

ssh="ssh -o BatchMode=yes -o ChallengeResponseAuthentication=no -o StrictHostKeyChecking=no"

while getopts s:p:c:dm opt 2>&-
do
	case $opt in
	s)	s_systemname="$OPTARG"
		[ $aflg ] && errflg=y
		sflg=y
		;;
	p)	appspass="$OPTARG"
		pflg=y
		;;
	c)	typeset -u cp_user="$OPTARG"
		cflg=y
		;;
	d)	dflg=y
		;;
	m)	mflg=y
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ "$cflg" -a ! "$pflg" ] && errflg=y
[ ! "$sflg" ] && errflg=y
[ $# -ne 0 ] && errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

username=`id -un`

setvars() {
	OPTIND=1
	typeset opt _fflg= _file= _errflg=
	while getopts f: opt 
	do
		case $opt in
		f)	_file="$OPTARG"
			_fflg=y
			;;
		\?) _errflg=y
		esac
	done
	shift `expr $OPTIND - 1`

	[ $_fflg ] || _errflg=y
	[ $_errflg ] && { echo "usage: setvars -f file var..." >&2; exit 2; }

	for i in $*
	do
		eval `ctxvar -ifv $i $_file`
	done
}

# Applications context
if appctxfile=`getctx -a -s $s_systemname 2>&-`
then
	setvars -f $appctxfile s_cp_user s_cp_password_type s_cp_resp_shortname s_cp_resp_name s_apps_user s_dbhost s_dbuser

	if [ ! "$dbctxfile" -a "$username" != "$s_dbuser" ]
	then
		echo "$prog: current user not owner of database tier (s_dbuser=$s_dbuser)" >&2
		exit 1
	fi
fi

uproc=createuser
rproc=addresp
pfunc=changepassword

if [ "$appctxfile" -a "$pflg" ]
then
	# check connection
	setvars -f $appctxfile s_tools_oh s_tools_tnsadmin s_tools_twotask
	if ORACLE_HOME=$s_tools_oh TNS_ADMIN=$s_tools_tnsadmin TWO_TASK=$s_tools_twotask oiconnect "${s_apps_user:-apps}/$appspass"
	then
		: 
	else
		echo "$prog: cannot connect to database as $s_apps_user" >&2
		exit 1
	fi
	cmd="ORACLE_HOME=$s_tools_oh TNS_ADMIN=$s_tools_tnsadmin TWO_TASK=$s_tools_twotask $s_tools_oh/bin/sqlplus -s"
	unpw="${s_apps_user:-apps}/$appspass"
	eval SQLPATH=BASEDIR/share/lib/sql $cmd <<-!
		$unpw
		whenever sqlerror exit failure
		whenever oserror exit failure

		set linesize 32767 pagesize 0
		set feedback off trimout on trimspool on termout off heading off

		@$uproc.plb
		@$rproc.plb
		@$pfunc.plb
	!
fi

if dbctxfile=`getctx -d -s $s_systemname 2>&-`
then
	setvars -f $dbctxfile s_db_oh s_dbSid
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
		echo "$prog: cannot connect to database as sysdba" >&2
		exit 1
	fi
	cmd="ORACLE_HOME=$s_db_oh ORACLE_SID=$s_dbSid $s_db_oh/bin/sqlplus -s"
	unpw="/ as sysdba"
fi

: ${cp_user:=$s_cp_user}
if [ $cflg ]
then
	# defaults
	# s_cp_user=SYSADMIN
	# s_cp_password_type=AppsSchema
	# s_cp_resp_shortname=SYSADMIN
	# s_cp_resp_name='System Administrator'
	s_cp_password_type="AppsUser"
	s_cp_resp_shortname="FND"
	s_cp_resp_name="Concurrent Manager Operator"
fi

if [ "$cp_user" = "SYSADMIN" ]
then
	echo "$prog: cp_user is SYSADMIN, specify a new user with -c" >&2
	exit 1
fi

if [ "$appctxfile" -a ! "$dflg" ]
then
	if $ssh -q -n $s_dbhost exit
	then
		if $ssh -q -n $s_dbhost "[ -x BINDIR/$prog ]"
		then
			:
		else
			echo $prog: BINDIR/$prog not present on $s_dbhost, install >&2
			exit 1
		fi
	else
		echo $prog: cannot connect to s_dbhost $s_dbhost, set up ssh login >&2
		exit 1
	fi
	#$ssh -n $s_dbhost BINDIR/$prog -s $s_systemname ${cflg:+-c $cp_user} -d
	#$ssh -n $s_dbhost BINDIR/$prog -s $s_systemname -d
	$ssh -n $s_dbhost ~/bin/$prog -s $s_systemname -d

	ctxvar -e s_cp_user=$cp_user \
		-e s_cp_password_type=$s_cp_password_type \
		-e s_cp_resp_shortname=$s_cp_resp_shortname \
		-e s_cp_resp_name="$s_cp_resp_name" \
		$appctxfile
	exit
fi

if [ $mflg ]
then
	rand="`makekey < /dev/random`"
else
	rand=`printf "%X!%x?%X\n" $RANDOM $RANDOM $RANDOM | sed -e 's;\(.\)\1\1;\1;g' -e 's;\(.\)\1;\1;g'`
fi

if eval $cmd <<-!
	$unpw

	whenever sqlerror exit failure
	whenever oserror exit failure

	set linesize 32767 pagesize 0
	set feedback off trimout on trimspool on termout off heading off

	variable n number

	BEGIN
		:n := 2;
		SELECT 1 INTO :n
		FROM applsys.fnd_user
		WHERE user_name = '$cp_user';
	EXCEPTION
		WHEN NO_DATA_FOUND THEN
		:n := 0;
	END;
	/
	exit :n
	!
then
	# user does not exist - create him
	if eval $cmd <<-!
		$unpw

		whenever sqlerror exit failure
		whenever oserror exit failure

		set linesize 32767 pagesize 0
		set feedback off trimout on trimspool on termout off heading off

		variable n number

		BEGIN
			:n := 0;
			SELECT 1 INTO :n
			FROM dba_objects
			WHERE owner = UPPER('${s_apps_user:-apps}')
			AND object_type = 'PROCEDURE'
			AND object_name = UPPER('XX$uproc');

			apps.xx$uproc(user_name => '$cp_user', description => '$s_cp_resp_name');
			apps.xx$rproc(user_name => '$cp_user', resp_shortname => '$s_cp_resp_shortname', resp_name => '$s_cp_resp_name');
		EXCEPTION
			WHEN NO_DATA_FOUND THEN
			:n := 2;
			WHEN OTHERS THEN
			:n := 1;
		END;
		/
		exit :n
	!
	then
		:
	elif [ $? -eq 2 ]
	then
		echo $prog: objects need to be created first, rerun with apps password >&2; exit 2
	else
		echo "SQL*Plus command failed" >&2; exit 1
	fi
elif [ $? -eq 2 ]
then
	echo "SQL*Plus command failed" >&2; exit 1
else
	# user exists - change his password
	if eval $cmd <<-!
		$unpw

		whenever sqlerror exit failure
		whenever oserror exit failure

		set linesize 32767 pagesize 0
		set feedback off trimout on trimspool on termout off heading off

		variable n number

		BEGIN
			:n := 1;
			SELECT 1 INTO :n
			FROM dba_objects
			WHERE owner = UPPER('${s_apps_user:-apps}')
			AND object_type = 'FUNCTION'
			AND object_name = UPPER('XX$pfunc');

			IF apps.xxchangepassword('$cp_user', '$rand')
			THEN
				:n := 0;
			END IF;
		EXCEPTION
			WHEN NO_DATA_FOUND THEN
			:n := 2;
			WHEN OTHERS THEN
			:n := 1;
		END;
		/
		exit :n
	!
	then
		:
	elif [ $? -eq 2 ]
	then
		echo $prog: objects need to be created first, rerun with apps password >&2; exit 2
	else
		echo "SQL*Plus command failed" >&2; exit 1
	fi
fi

if [ ! "$appctxfile" -a ! "$dflg" ]
then
	exit 1
fi
echo $cp_user/$rand

exit 0
