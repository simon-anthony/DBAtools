#ident $Header: DBAtools/trunk/share/lib/functions/setoavars.sh 187 2017-11-16 15:25:59Z xanthos $
# vim:syntax=sh:ts=4:sw=4:
################################################################################
# setvars: set context variables from supplied context file name
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
setvars()
{
	#OPTIND=1
	typeset -l _fflg= _vflg= _errflg=
	typeset _var= opt=
	typeset prefix="/opt/oracle"

	while getopts f:v opt $* 2>&-
	do
		case $opt in
		f)	_fflg=y; _ctxfile="$OPTARG"	# context file
			;;
		v)	_vflg=y;					# verbose
			;;
		\?)	_errflg=y
		esac
	done
	shift `expr $OPTIND - 1`

	[ "$_fflg" ] || _errflg=y

	if [ $_errflg ]
	then
		print -u2 "usage: setvars [-v] -f <context_file> [<var>]..."
		return 2
	fi

	_errflg=0
	for _var in *
	do
		if eval `$prefix/bin/ctxvar -ifv $_var $_ctxfile`
		then
			[ $_vflg ] && eval print "setvars: context variable $_var for is \$$_var"
		else
			print -u2 "setvars: cannot get context variable $_var"
			(( _errflg += 1 ))
		fi
	done

	return $_errflg
}
typeset -fx setvars
