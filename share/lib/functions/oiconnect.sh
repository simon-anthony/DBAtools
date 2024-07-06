#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# oiconnect: test connection to oracle
#  - really should use a parser
#   $1 - username/passwd
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
oiconnect()
{
	expr "$1" : '[^/]\{1,\}/[^/]\{1,\}' \| "$1" : '/[ 	]\{1,\}[Aa][Ss][ ]\{1,\}[A-Za-z]\{1,\}' >/dev/null || {
		echo usage: oiconnect username/password >&2
		return 2
	}
	[ $ORACLE_HOME ] || {
		echo oiconnect: ORACLE_HOME not set >&2; return 2; }
	[ -z "$ORACLE_SID" -a -z "$TWO_TASK" ] && {
		echo oiconnect: ORACLE_SID or TWO_TASK must be set >&2; return 1; }
	[ -x $ORACLE_HOME/bin/sqlplus ] || {
		echo oiconnect: sqlplus not found >&2; return 1; }

	$ORACLE_HOME/bin/sqlplus -s <<-! >/dev/null 2>&1
		$1
		whenever sqlerror exit failure
		whenever oserror exit failure
		SELECT 1 from DUAL;
		exit success
	!
	return $?
}
typeset -fx oiconnect
