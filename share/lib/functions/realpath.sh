#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# realpath: canonicalize the supplied path
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
realpath()
{
	[ $# -eq 1 ] || { echo "usage: realpath <path>" >&2; return 1; }

        if expr "$1" : '.*/.*' >/dev/null 2>&1  # has directory component
        then
                dir=`dirname $1`
                ( cd $dir 2>&- && echo `pwd`/`basename $1` )
        else                                    # just filename
                echo `pwd`/$1
	fi
}
typeset -fx realpath
