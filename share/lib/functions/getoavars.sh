#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# getoavars: extract s_ variables from a file
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
getoavars()
{
	for file
	do
		awk '/^[^#].*%[a-zA-Z0-9_]*%/ {
			while (index($0, "%") != 0) {
				$0 = substr($0, index($0, "%")+1)
				s = sprintf("%s", substr($0, 1, index($0, "%")-1))
				if (match(s, /^s_/))
					print s
				
				$0 = substr($0, index($0, "%")+1)
			}
		}' $file
	done | sort -u
}
typeset -fx getoavars
