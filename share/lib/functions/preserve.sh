#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# preserve: make a backup copy of $1 - appending 1, 2, 3 ...
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
preserve()
{
	[ "$1" ] || return 2
	[ -f "$1" ] || { echo preserve: $1 non-existent >&2; return 1; }
	_i=
	if [ -f $1.bak ]
	then
		if [ -f $1.bak.1 ]
		then
			_i=`ls $1.bak.?([0-9]*) |
			sed 's;.*\.\([0-9]\{1,\}\)$;\1;p'| sort -nr | sed 1q`
			_i=`expr $_i + 1`
		else
			_i=1
		fi
	fi
	cp $1 $1.bak${_i:+.$_i}
}
typeset -fx preserve
