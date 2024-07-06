#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# xmlvar: return value of xml variable in context file, really should use a
#         parser
#   $1 - variable name  $2 - XML file
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
xmlvar()
{
	[ $# -eq 2 ] || {
		echo "usage: xmlvar <variable name> <XML context file>" >&2;
		return 1; }
	[ -r "$2" ] || { echo "xmlvar: cannot open $2" >&2; return 1; }

	_ctx=`ident $2 2> /dev/null | awk '
		/Header:/ { print substr($2, 1, match($2, /\./)-1); exit }'`

	case $_ctx in
	adxdbctx) : ;; # Apps context file
	adxmlctx) : ;; # DB context file
	*)	echo "xmlvar: invalid XML context file '$2'"; return 1;
	esac

	sed -n 's;.*[ 	]\{1,\}oa_var="'$1'"[^>]*>\(.*\)<.*;\1;p' $2
}
typeset -fx xmlvar
