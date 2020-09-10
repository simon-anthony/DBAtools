#ident $Header$
# vim:syntax=sh:ts=4:sw=4:
################################################################################
# msg: save or restore stdout/stderr or print message
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
msg()
{
	#OPTIND=1
	typeset -l _fflg= _vflg= _oflg= _eflg= _pflg= _dflg= _hflg= _nflg= _tflg= _errflg=
	typeset _dt= _host= _tt= 

	while getopts f:dvoephnt opt $* 2>&-
	do
		case $opt in
		f)	_fflg=y; _name="$OPTARG"	# save stdio
			[ "$_vflg" -o "$_oflg" -o "$_eflg" ] && _errflg=y ;;
		v|r)	_vflg=y; _name=""			# restore stdio to prior
			[ "$_fflg" -o "$_oflg" -o "$_eflg" ] && _errflg=y ;;
		o)	_oflg=y; 					# current stdout
			[ "$_fflg" -o "$_vflg" -o "$_eflg" ] && _errflg=y ;;
		e)	_eflg=y; 					# current stderr
			[ "$_fflg" -o "$_vflg" -o "$_oflg" ] && _errflg=y ;;
		p)	_pflg=y;					# previous stdout/stderr
			[ "$_fflg" -o "$_vflg" ] && _errflg=y ;;
		d)	_dflg=y
			_dt="`/bin/date '+%d-%b-%Y %H:%M:%S - '`" ;;
		t)	_tflg=y
			_tt=$_prog: ;;
		h)	_hflg=y
			_host=`hostname` ;; 
		n)	_nflg=y;;					# like print -n	
		\?)	_errflg=y
		esac
	done
	shift `expr $OPTIND - 1`

	[ "$_pflg" -a ! \( -n "$_oflg" -o -n "$_eflg" \) ] && _errflg=y

	if [ $_errflg ]
	then
		print -u2 "usage: msg [-f <name>|-v|-o[p]|-e[p]] [-d][-h][-t] [<arg>]..."
		return 2
	fi
	_dir=${LOCALSTATEDIR:=$HOME/admin/log}
	mkdir -p $LOCALSTATEDIR 2>&- || {
		print -u2 "msg: cannot create $LOCALSTATEDIR"
		return 1
	}

	if [ $_fflg ]	# save stdout/stderr
	then
		if [ "A$prog" != "A" ]
		then
			_prog=$prog
		else
			_prog=`basename $0`
		fi
		if [ ! "$_saved_stdio" ]
		then
			exec 3>&1 1>$_dir/$_name.out 4>&2 2>$_dir/$_name.err || return 1
		fi
		_saved_stdio=y
	elif [ $_vflg ]	# restore stdout/stderr
	then
		if [ $_saved_stdio ]
		then
			exec 1>&3 2>&4 || return 1
		fi
		_saved_stdio=
	elif [ $_oflg ]	# print to "current" stdout
	then
		if [ "$_pflg" -a "$_name" ]	# print to "saved" stdout
		then
			print ${_nflg:+-n} $_dt $_tt ${_host:+$_host: } "$*" >&3 && return 0
		fi
	elif [ $_eflg ]	# print to "current" stderr
	then
		if [ "$_pflg" -a "$_name" ]	# print to "saved" stderr
		then
			print ${_nflg:+-n} $_dt $_tt ${_host:+$_host: } "$*" >&4 && return 0
		fi
		print ${_nflg:+-n} $_dt $_tt ${_host:+$_host: } "$*" >&2 && return 0
	fi

	[ $# -gt 0 ] && print ${_nflg:+-n} $_dt $_tt ${_host:+$_host: } "$*"
	return 0
}
typeset -fx msg
