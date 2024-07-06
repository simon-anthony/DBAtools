#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# appspass: get apps password
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
#
appspass()
{
	[ $# -ne 1 ] && { echo "usage: $prog <path to apps context file>" >&2; return 2; }
	[ -r "$1" ] || { echo "$prog: cannot access $1" >&2; return 1; }

	if ident $1 | grep -q 'Header: adxmlctx'
	then
		# e.g. /u20/app/DINO/apps/admin/DINO_ukgtjd35.xml
		appctxfile=`realpath $1`
	else
		echo "$prog: $1 is not a valid application node context file" >&2
		return 1
	fi

	_s_dbSid=`xmlvar s_dbSid $appctxfile`
	_s_weboh_oh=`xmlvar s_weboh_oh $appctxfile`

	if [ -r $_s_weboh_oh/Apache/modplsql/cfg/wdbsvr.app ]
	then
		awk '
		/\[DAD_'$_s_dbSid']/,/;/ {
			if ($1 == "username")
				username = tolower($3)
			if ($1 == "password")
				password = tolower($3)
		}
		END {
			#print username "/" password
			print password
		}' $_s_weboh_oh/Apache/modplsql/cfg/wdbsvr.app

	fi
}
typeset -fx appspass
