#ident $Header: DBAtools/trunk/share/lib/functions/sqlprompt.sh 133 2017-08-24 16:43:46Z xanthos $
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# sqlprompt: Print SQL*Plus commands to set the prompt
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
sqlprompt()
{
	typeset _colour= _columns= _lines=

	if [ "$TWO_TASK" -a "$TWO_TASK" != NULL ]
	then
		_colour=${SG_TWO_TASK:-red}
	elif [ $ORACLE_SID ]
	then
		_colour=${SG_ORACLE_SID:-green}
	fi
	_columns=${COLUMNS:-`tput cols 2>&-`}
	_lines=${LINES:-`tput cols 2>&-`}
	echo set linesize ${_columns:-80} pagesize ${_lines:-14}
	echo set sqlprompt \"$(sg $_colour)$1$(sg off)\> \"
}
typeset -fx sqlprompt
