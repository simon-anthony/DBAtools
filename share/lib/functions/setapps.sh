#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# setapps: set apps environment
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
setapps()
{
	#OPTIND=1
	typeset -u _sid="`/usr/bin/expr  \`/usr/bin/id -un\` : '...\(.*\)'`" \
				un=`/usr/bin/id -un`
	typeset s_systemname=$_sid
	typeset opt= sflg= hflg= errflg= \
		    ctxfile= s_apps_version= s_inst_base= s_appsora_file= 
	typeset prog="setapps" prefix="/opt/oracle"

	while getopts s:h opt $* 2>&-
	do
		case $opt in
		s)	s_systemname="$OPTARG"
			sflg=y;;
		h)	errflg=y ;;
		\?)	errflg=y
		esac
	done
	shift $(( OPTIND - 1 ))

	[ $# -gt 1 ] && errflg=y

	if [ $# -eq 1 ]
	then
		case $1 in
		run|patch) : ;;
		*) errflg=y
		esac
	fi

	[ $errflg ] && { print -u2 "usage: $prog [-s s_systemname] [run|patch]"; return 1; }

	if ctxfile=`$prefix/bin/ctx -anfs $s_systemname 2>/dev/null`
	then
		eval `$prefix/bin/ctxvar -ifv s_apps_version $ctxfile`
	elif [ ! "$sflg" ] && ctxfile=`$prefix/bin/ctx -anfs $un 2>/dev/null`
	then
		 eval `$prefix/bin/ctxvar -ifv s_apps_version $ctxfile`
	else
		print -u2 "$prog: cannot locate context file for $s_systemname ${sflg:-or $un}"
		return 1
	fi

	if [ "${s_apps_version%.*}" = "12.2" ]
	then
		eval `$prefix/bin/ctxvar -ifv s_inst_base $ctxfile`
		. $s_inst_base/EBSapps.env ${1:-run}
	else
		eval `$prefix/bin/ctxvar -ifv s_appsora_file $ctxfile`
		. $s_appsora_file
	fi
	PS1=`$prefix/bin/pset -th -T`
}
typeset -fx setapps
