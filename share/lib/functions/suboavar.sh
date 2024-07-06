#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# suboavar: substitute s_ or c_ variable from a file
#   $1  - variable
#   $2  - value
#  [$3] - file - or stdin
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
suboavar()
{
	awk '/^[^#].*%[a-zA-Z0-9_]*%/ {
		gsub(%'"$1"', '"$2"', $0)
	}' $1
}
typeset -fx suboavar
