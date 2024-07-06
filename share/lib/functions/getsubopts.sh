#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# getsubopts:
# Function is similar to getsubopt(3C), argument order differs. Note that
# arguments 2, 3 and 4 are the *names* of variables to which values will be
# assigned (or overridden).
#  $1 literal list of tokens
#  $2 name of shell variable that will have assigned the current option name
#  $3 name of shell variable that will have assigned the current option value
#  $4 name of shell variable that will have assigned the arguments to process
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
getsubopts()
{
	# Example:
	#
	# passwords="apps=EWFWEF9787 ,system= lwecfwehcb,bert=smith"
	# 
	# while getsubopts "apps system" opt value passwords
	# do
	# 	case $opt in
	# 	"apps")	echo username apps password is $value
	# 		;;
	# 	"system")	echo username system password is $value
	# 		;;
	# 	\?) echo invalid option
	# 	esac
	# done
	[ $# -eq 4 ] || {
		print -u2 "usage: getsubopt <tokens> <optstring> <value> [<arg>...]"
		print -u2 "       args=\"apps=EWFWEF9787,system=lwecfwehcb\""
		print -u2 "       getsubopt "apps system" opt value args"
		return 2; }
	typeset tokens="$1" optp="$2" valuep="$3" argsp="$4" 
	typeset nv= n= v= found=

	if eval test \"X\$$argsp\" = "X" 
	then
		return 1
	fi

	eval nv=\$\{$argsp%%,*\}
	if eval test \"X\$$argsp\" = \"X\$\{$argsp#*,\}\"
	then
		eval $argsp=
	else
		eval $argsp=\$\{$argsp#*,\}
	fi

	n=${nv%=*}

	for t in $tokens
	do
		[ "$n" = "$t" ] && { found=y; break; }
	done
	[ "$found" ] || {
		eval $optp=\?
		print -u2 "getsubopt: invalid option '$n'"
		return 0
	}

	v=${nv#*=}

	if [ "$n" = "$v" ]
	then
		eval $valuep=
	else
		eval $valuep=\"$v\"
	fi
	eval $optp="$n"

	return 0
}
typeset -fx getsubopts
