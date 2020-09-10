#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# adenv: locate and source environment files for Oracle e-Business Suite
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

# _SG: print vt220 graphics modes
#
_SG() {
	typeset _pi= _p=
	while [ $1 ]
	do
		case $1 in
		black)	_p=30 ;;
		red)	_p=31 ;;
		green)	_p=32 ;;
		yellow)	_p=33 ;;
		blue)	_p=34 ;;
		magen*)	_p=35 ;;
		cyan)	_p=36 ;;
		white)	_p=37 ;;
		ul)		_p=4  ;;
		off)	_p=0  ;;
		*)		shift; continue ;;
		esac
		shift
		[ "$_pi" ] && _pi="$_pi;$_p" || _pi=$_p
	done
	echo "\033[${_pi}m\c"
}
_ps() {
	OPTIND=1
	typeset _sflg= _tflg= _uflg=
	typeset _var=ORACLE_SID 
	typeset _col=${SG_ORACLE_SID:-green}
	typeset __hostname= __ps=
	while getopts stu opt $* 2>&-
	do
		case $opt in
		t)		_tflg=y; _var=TWO_TASK; _col=${SG_TWO_TASK:-red} ;;
		u)		_uflg=y ;;
		s)		_sflg=y; ;;
		esac
	done
	shift `expr $OPTIND - 1`

	__hostname=`hostname`
	__hostname=${__hostname%%.*}

	if [ $_hflg ]
	then
		__ps=${HOSTNAME:-$__hostname}@$LOGNAME
	else
		__ps=$LOGNAME
	fi
	if [ $_Tflg ]
	then
		__ps1=']0;'$LOGNAME@$__hostname':$PWD'$__ps'${'$_var':+['`_SG $_col ${_uflg:+ul}`'$'$_var`_SG off`']}\$'
	else
		__ps1=$__ps'${'$_var':+['`_SG $_col ${_uflg:+ul}`'$'$_var`_SG off`']}\$'
	fi
	echo $__ps1
}

# _xmlvar: return value of xml variable in context file
#  - really should use a parser
#   $1 - variable name  $2 - XML file
#
_xmlvar()
{
	[ -r "$2" ] || { echo "xmlvar: cannot open $2" >&2; exit 1; }
	sed -n 's;.*[   ]\{1,\}oa_var="'$1'"[^>]*>\(.*\)<.*;\1;p' $2
}


# _adenv: usage: adenv s_systemname - determine environment settings for database s_systemname.
#
_adenv() {
	set +u 
	typeset _errflg= _prog= _usage= _kusage= _shell=
	_prog=`[ \`uname -s\` = Linux ] && basename -- $0 || basename $0`	# as this file is sourced, this will be the name of a shell

	# Files can only be sourced with args in ksh and derivatives.
	_usage=""
	_kusage="[-v] [-a[-x]|-d[-ow]|-i[-o]] [-n] [-p] [-h] [-t] [<database_name>]"

	[ $_prog = adenv ] && _errflg=y	# file has not been sourced

	typeset _shell=`UNIX95=1 ps -p $$ -o args | sed -n '2{ s/^\([^ 	]*\).*/\1/p; q;}'`
	typeset _prog=adenv		# set prog to name of this file

	case $_shell in
	sh|-sh|/bin/sh|/usr/bin/sh)
		if [ `uname -s` = "HP-UX" ]
		then
			_usage=$_kusage
			_shell=ksh	# HP-UX POSIX shell, roughly equivalent to ksh
		else
			_shell=sh	# other systems: Bourne shell
		fi
		;;
	/usr/old/bin/sh)
		_shell=sh		# HP-UX Bourne shell
		;;
	ksh|-ksh|*/bin/ksh|bash|-bash|*/bin/bash)
		_usage=$_kusage
		_shell=ksh
		;;
	*)	echo $_prog: invalid shell $_shell >&2
		return 1
	esac

	if [ `uname -s` = "HP-UX" ]
	then
		[ -r /etc/rc.config.d/oracle ] && . /etc/rc.config.d/oracle
	else
		[ -r /etc/default/oracle ] && . /etc/default/oracle
	fi
	[ "$HOSTNAME" ] && _host=$HOSTNAME export HOSTNAME

	OPTIND=1		# sh does not re-initialise OPTIND
	typeset _dflg= _aflg= _nflg= _pflg= _vflg= _hflg= _oflg=y _cflg= _xflg= _iflg= _wflg=
	while getopts doanipvhxc:tw opt 2>&-
	do
		case $opt in
		d)	_dflg=y
			[ "$_aflg" -o "$_iflg" ] && _errflg=y
			;;
		o)	_oflg=y		# ignore apps env file when -d flag
			;;
		a)	_aflg=y
			_oflg=
			[ "$_dflg" -o "$_iflg" ] && _errflg=y
			;;
		n)	_nflg=y		# don't override ORACLE_SID
			;;
		p)	_pflg=y		# re-read profiles
			;;
		v)	_vflg=y		# verbose
			;;
		h)	_hflg=y		# put hostname in PS1
			;;
		t)	_Tflg=y		# set terminal title
			;;
		w)	_wflg=y		# warn for non-existant oratab entries
			;;
		x)	_xflg=y		# suppress search for "APPSORA" file
			;;
		i)	_iflg=y		# iAS ORACLE_HOME
			[ "$_dflg" -o "$_aflg" ] && _errflg=y
			;;
		c)	_cflg=y		# machine is a cluster - use to generate "context"
			if /usr/sbin/ping $OPTARG >/dev/null 2>&1
			then
				_host=$OPTARG
			else
				echo $_prog: host \'$OPTARG\' is down or name does not exist >&2
				return 1
			fi
			;;
		\?)	_errflg=y
		esac
	done
	shift `expr $OPTIND - 1`

	[ "$_oflg" -a ! \( -n "$_dflg" -o -n "$_iflg" \) ] &&
		_errflg=y	# must have -d|-i if -o

	[ "$_xflg" -a ! "$_aflg" ] && _errflg=y		# must have -a if -x
	[ "$_wflg" -a ! "$_dflg" ] && _errflg=y		# must have -d if -w

	# HP-UX Bourne shell allows args to sourced files but getopts will not
	# trap invalid options in this instance
	if [ $_shell = sh ]
	then
		[ $# -gt 0 ] && _errflg=y
	fi

	if [ $_errflg ]
	then
		if [ $_shell = sh ]
		then
			echo "usage: [ORACLE_SID=s_systemname] [TWO_TASK=s_systemname] . $_prog $_usage" >&2
		else
			echo "usage: . $_prog $_usage" >&2
		fi
		return 2
	fi

	[ $TWO_TASK ] && _sid=${1:-$TWO_TASK} || _sid=${1:-$ORACLE_SID}

	if [ ! "$1" ]
	then
		if [ ! "$_aflg" -a ! "$_iflg" -a ! "$_dflg" ]
		then
			if [ $TWO_TASK ]
			then
				_aflg=y
			elif [ $ORACLE_SID ]
			then
				_dflg=y
			fi
		fi
		_oflg=y		# assume I don't want apps environment
	else
		[ ! "$_aflg" -a ! \( -n "$_dflg" -o -n "$_iflg" \) ] &&
			_aflg=y								# -a is default
	fi

	[ $_sid ] || { echo ORACLE_SID or TWO_TASK not set >&2; return 1; }

	if [ "$ORACLE_HOME" ]
	then
		# remove ORACLE_HOME from PATH
		PATH=`echo $PATH | sed "s;$ORACLE_HOME/[^:]*;;g"`
		[ "$LD_LIBRARY_PATH" ] && LD_LIBRARY_PATH=`echo $LD_LIBRARY_PATH | sed "s;$ORACLE_HOME/[^:]*;;g"`
		[ "$SHLIB_PATH" ] && SHLIB_PATH=`echo $SHLIB_PATH | sed "s;$ORACLE_HOME/[^:]*;;g"`
	fi
	unset ORA_NLS10 ORA_NLS ORACLE_HOME TNS_ADMIN TWO_TASK ORACLE_SID SQL_PATH ORACLE_PATH

	# Figure out what sort of node we have
	#
	eval `BINDIR/ctx ${_oflg:+-d} -f -s $_sid 2>&-`
	typeset _appctxfile= _dbctxfile=
	_appctxfile=$APPCTX
	_dbctxfile=$DBCTX
	eval `BINDIR/ctx ${_oflg:+-d} -s $_sid 2>&-`

	if [ "$_aflg" -o ! "_$dflg" ]
	then
		if [ ! -r "$_appctxfile" ]
		then 
			echo $_prog: cannot locate any XML file at APPL_TOP for $_sid >&2
			return 1
		fi
	fi
	if [ "$_dflg" ]
	then
		if [ ! -r "$_dbctxfile" ]
		then 
			echo $_prog: cannot locate any XML file for database for $_sid >&2
			return 1
		fi
	fi

	# Check for env files
	#
	typeset _dbenv= _applenv=
	if [ $_dflg ]
	then
		_dbenv=`BINDIR/ctxvar -if s_db_home_file $_dbctxfile`		# database
	else
		_dbenv=`BINDIR/ctxvar -if s_tools_home_file $_appctxfile`
	fi

	if [ \( \( -n "$_dflg" -o -n "$_iflg" \) -a -z "$_oflg" \) -o -n "$_aflg" ]
	then
		if [ -n "$_dflg" -a -z "$_appctxfile" ]
		then
			:
		else
			_applenv=`BINDIR/ctxvar -if s_applsys_file $_appctxfile` # APPL_TOP
		fi
	fi

	if [ "$_iflg" ]
	then
		_dbenv=`BINDIR/ctxvar -if s_web_home_file $_appctxfile`
	fi

	# Reset PATH to that from profiles
	#
	if [ $_pflg ]
	then
		PATH=`(
			. /etc/profile >/dev/null 2>&1
			. $HOME/.profile >/dev/null 2>&1
			echo $PATH
			exec >/dev/null 2>&1
			)`
		export PATH
		expr "$PATH" : '.*:BINDIR' >/dev/null || \
			PATH=$PATH:BINDIR export PATH
	fi

	# Source env files
	#
	[ $_vflg ] && echo Environment file\(s\):
	source() {
		[ -r $1 ] || {
			echo $_prog: cannot locate \'$1\' >&2
			return 1
		}
		[ $_vflg ] && echo " $1"
		. $1
	}

	typeset _appsora= _appsoraenv= _customenv=
	if [ ! "$_dflg" -a ! "$_iflg" ]	# first check for "APPSORA" for TWO_TASK
	then
		_appsoraenv=`BINDIR/ctxvar -if s_appsora_file $_appctxfile`
		if [ ! "$_xflg" -a  -r $_appsoraenv ]
		then
			_appsora=y
			_applenv=$_appsoraenv
		fi
	fi
	[ \( -n "$_dflg" -o -n "$_iflg" \) -a -n "$_oflg" ] && _appsora=
	[ $_appsora ] || {
		if [ -n "$_oflg" -o -z "$_appctxfile" ]
		then
			:
		else
			_customenv=`BINDIR/ctxvar -if s_custom_file $_appctxfile`
			[ -r $_customenv -a ! "$_oflg" ] &&
				source $_customenv
		fi
		source $_dbenv
	}
	[ \( \( -n "$_dflg" -o -n "$_iflg" \) -a -z "$_oflg" -a -r "$_applenv" \) -o -n "$_aflg" ] &&
		source $_applenv

	if [ $TWO_TASK ]
	then
		# Some Apps programs get upset if ORACLE_SID is set when TWO_TASK is set
		# However, the respective .env file *should* sort this out.
		if [ "$_nflg" -a ! "$ORACLE_SID" ]
		then
			ORACLE_SID=$_sid export ORACLE_SID
		else
			unset ORACLE_SID
		fi
		if [ -x BINDIR/pset ]
		then
			PS1="`BINDIR/pset -T ${hflg:+-h} -t` "
		else
			PS1="`_ps -t -h` "
		fi
	elif [ $ORACLE_SID ]
	then
		unset TWO_TASK
		if [ -x BINDIR/pset ]
		then
			PS1="`BINDIR/pset -T ${hflg:+-h} -s` "
		else
			PS1="`_ps -s -h` "
		fi
	fi
	if [ $_shell = sh ]
	then
		PS1=`eval echo $PS1`
	fi
	export PS1


	if [ $_vflg ]
	then
		echo Environment set: `basename $ORACLE_HOME`
		[ $TWO_TASK ] &&       echo \ TWO_TASK\ \ \  $TWO_TASK || {
			[ $ORACLE_SID ] && echo \ ORACLE_SID\  $ORACLE_SID; }
		[ $ORACLE_HOME ] &&      echo \ ORACLE_HOME $ORACLE_HOME
		[ \( \( -n "$_dflg" -o -n "$_iflg" \) -a -z "$_oflg" \) -o -n "$_aflg" ] &&
			echo \ APPL_TOP\ \ \  $APPL_TOP

		# Is there also an entry in oratab? Only the first is printed.

		if [ "$_dflg" -a "$_wflg" ]
		then
			typeset _oratab_oracle_home= _oratab=
			if _oratab_oracle_home=`SBINDIR/oratab -p $ORACLE_SID`
			then
				_oratab_oracle_home=`echo $_oratab_oracle_home | cut -f2 -d:`
				_oratab=`ls /var/opt/oracle/oratab /etc/oratab 2>&-`
				[ -d "$_oratab_oracle_home" ] || {
					cat - <<-! >&2 
						$_prog: WARNING entry for ORACLE_HOME in $_oratab:
						  '$_oratab_oracle_home'
						is an inaccessible directory
					!
				}
				# Check that it matches the RapidInstall ORACLE_HOME
				if [ "$_oratab_oracle_home" != $ORACLE_HOME ]
				then
					cat - <<-! >&2 
						$_prog: WARNING entry for ORACLE_HOME in $_oratab:
						  '$_oratab_oracle_home'
						does not match that configured by RapidInstall:
						  '$ORACLE_HOME'
					!
				fi
				unset _oratab_oracle_home _oratab
			else
				echo $_prog: WARNING no entry for ORACLE_HOME in $oratab >&2
			fi
		fi
	fi
	return 0
}

_flags=
case "$-" in
*u*)	set +u 
		_flags=u
		;;
esac

_rc=0
_adenv $* && { [ -n "$1" ] && shift; } || _rc=$?

[ "$_flags" ] && set +u

[ $_rc -eq 0 ] && true || false
