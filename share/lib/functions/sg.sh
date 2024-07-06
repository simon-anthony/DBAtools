#!/usr/bin/ksh

# vim:syntax=sh:sw=4:ts=4:
################################################################################
# sg: print vt220 graphics modes, used by sqlprompt()
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
sg()
{
	typeset _pi= _p=
	while [ $1 ]
	do
		case ${1%%:*} in
		black)	_p=30 ;;
		red)	_p=31 ;;
		green)	_p=32 ;;
		yellow)	_p=33 ;;
		blue)	_p=34 ;;
		magen*)	_p=35 ;;
		cyan)	_p=36 ;;
		white)	_p=37 ;;
		ul|un*)	_p=4  ;;
		bold)	_p=1  ;;
		off)	_p=0  ;;
		*)		shift; continue ;;
		esac
		shift
		[ "$_pi" ] && _pi="$_pi;$_p" || _pi=$_p
	done
	echo "\033[${_pi}m\c"
}
typeset -fx sg
