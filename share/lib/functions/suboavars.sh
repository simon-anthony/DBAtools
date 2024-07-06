#!/usr/bin/ksh

# vim:syntax=sh:ts=4:sw=4:
################################################################################
# suboavars:  replace context variables in file or stdin with context values 
# taken from context file
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
suboavars()
{
	typeset opt= vflg= fflg= dflg=0 tflg=0 ctxfile= vals= file= \
		usage="usage: suboavars [-d|-t] [-v <var>=<val>...] -f <contextfile> [<file>]"

	while getopts f:v:dt opt 2>&-
	do
		case $opt in
		f)	ctxfile="$OPTARG"
			fflg=y
			;;
		v)	vals="$vals $OPTARG"
			vflg=y
			;;
		d)	[ $tflg -eq 1 ] && errflg=y
			dflg=1
			;;
		t)	[ $dflg -eq 1 ] && errflg=y
			tflg=1
			;;
		\?)	errflg=y ;;
		esac
	done
	shift `expr $OPTIND - 1`

	[ $# -gt 0 ] && errflg=y
	[ $fflg ] || errflg=y
	[ $errflg ] && { print -u2 "$usage"; return 2; }

	[ $# -eq 1 ] && file=$1

	awk -v dflg=$dflg -v ctxfile=$ctxfile '
		BEGIN {
			if (dflg) {	# driver file syntax
				lb = "<"; rb = ">"
			} else {
				lb = "%"; rb = "%"
			}
			pattern = sprintf("%c[cs]_[a-zA-Z0-9_]+%c", lb, rb)
		}
		/[<%][cs]_/ {
			line = $0
			split("", vars, ":")	# clear array

			while (match(line, pattern)) {
				s = substr(line, RSTART+1, RLENGTH-2)
				vars[n++] = s
				line = substr(line, 1 + RSTART + RLENGTH)
			}
			for (var in vars) {
				val = ""
				for (i = 0; i < ARGC; i++) {
					v = substr(ARGV[i], 1, index(ARGV[i], "=")-1)
					if (v == vars[var])	# override context file 
						val = substr(ARGV[i], index(ARGV[i], "=")+1)
						#printf("COMPARE %s: %s to %s\n", ARGV[i], v, vars[var])
				}
				if (length(val) == 0) {
					cmd = sprintf("ctxvar -if %s %s", vars[var], ctxfile)
					cmd | getline val
				}
				gsub(lb vars[var] rb, val)
			}
		}
		{ print }' $vals $file
}
typeset -fx suboavars
