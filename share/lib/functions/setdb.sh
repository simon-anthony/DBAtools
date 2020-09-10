#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# setdb: set database environment
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
setdb()
{
	#OPTIND=1
	typeset -u _sid="`/usr/bin/expr  \`/usr/bin/id -un\` : '...\(.*\)'`" \
			  un=`/usr/bin/id -un`
	typeset oracle_sid= two_task= 
	typeset opt= oflg= oh= oflg= tflg= sflg= bflg= errflg= \
		ctxfile= file= s_db_home_file= \
		oratab= tnsnames= \
		prog="setdb" prefix="/opt/oracle"

	while getopts bot: opt $* 2>&-
	do
		case $opt in
		b)	bflg=y
			;;
		o)	oflg=y
			[ $tflg ] && errflg=y
			;;
		t)	tflg=y 
			two_task="$OPTARG"
			[ $oflg ] && errflg=y
			;;
		\?)	errflg=y 
		esac
	done
	shift $(( OPTIND - 1 ))

	[ $# -gt 1 ] && errflg=y

	[ $errflg ] && { print -u2 "usage: $prog [-b] [-o|-t <two_task>] [<oracle_sid>]"; return 1; }

	if [ $# -eq 1 ] 
	then
		oracle_sid=$1
	else
		if [ "X$_sid" = "X" ]
		then
			print -u2 "$prog: cannot determine oracle_sid"
			return 1
		fi
		oracle_sid="$_sid"
	fi

	if [ "$oflg" -o "$tflg" ]
	then
		oratab=`ls /var/opt/oracle/oratab /etc/oratab 2>/dev/null`
		[ $oratab ] || { print -u2 "$prog: cannot find oratab"; return 1; }
		[ -r $oratab ] || { print -u2 "$prog: cannot read $oratab"; return 1; }

		[ "$tflg" -a -z "$oracle_sid" ] && oracle_sid="$two_task"

		if oh=`awk -F: '
			BEGIN { n=1; }
			$1 == "'$oracle_sid'" { print $2; n=0; }
			$1 == "+'$oracle_sid'" { print $2; n=0; }
			END { exit n; }' $oratab`
		then
			if [ ! -x "$oh/bin/sqlplus" ]
			then
				print -u2 "$prog: invalid ORACLE_HOME '$oh'"
				return 1
			fi

			if [ $oflg ]
			then
				if awk -F: '
					BEGIN { n=1; }
					$1 == "+'$oracle_sid'" { n=0; }
					END { exit n; }' $oratab 
				then
					ORACLE_SID=+$oracle_sid export ORACLE_SID	# ASM
				else
					ORACLE_SID=$oracle_sid export ORACLE_SID
				fi
				unset TWO_TASK
				[ -t ] && PS1=`$prefix/bin/pset -sh -T` export PS1
			else
				tnsnames=`ls $HOME/.tnsnames.ora ${TNS_ADMIN:-$oh/network/admin}/tnsnames.ora 2>/dev/null`

				[ "$tnsnames" ] || { print -u2 "$prog: cannot find tnsnames.ora"; return 1; }
				typeset found=
				for file in $tnsnames
				do
					[ -r $file ] || { print -u2 "$prog: cannot read $file"; return 1; }
					if awk '
						BEGIN { n = 1 }
						$1 ~ /^'$two_task'[=]*$/ { n = 0 }
						END { exit n }
					' $file
					then
						found=y
						break
					fi
				done
				if [ $found ]
				then
					TWO_TASK=$two_task export TWO_TASK
					unset ORACLE_SID
					[ -t ] && PS1=`$prefix/bin/pset -th -T` export PS1
				else
					print -u2 "$prog: alias '$two_task' not found in $tnsnames"
					return 1
				fi
			fi

			# clear out old ORACLE_HOMEs from PATH
			typeset ohs=`awk -F: '$1 ~ /^\+*[A-Za-z]+/ { printf("%s ", $2) }' $oratab`
			PATH=`echo $PATH | awk -v ohs="$ohs" -F: '
			BEGIN {
				split(ohs, a, " ")
			}
			{
				for (n = 1; n <= NF; n++) {
					if (!is_oh($n)) {
						if (i++ > 0) sep=":"
						printf("%s%s", sep, $n)
					}
				}
			}
			END {
				printf("\n")
			} 
			function is_oh(path,   i) {
				for (i in a)
					if (path == a[i] "/bin")
						return 1
				return 0
			}'`

			ORACLE_HOME=$oh
			[ $bflg ] && PATH=$ORACLE_HOME/bin:$PATH || PATH=$PATH:$ORACLE_HOME/bin 
			export ORACLE_HOME PATH 
			return 0
		else
			print -u2 "$prog: no ORACLE_HOME for '$oracle_sid' in $oratab"
			return 1
		fi
	fi

	if { ctxfile=`$prefix/bin/ctx -dnfs $oracle_sid` || ctxfile=`$prefix/bin/ctx -dnfs $un` ; } 2>/dev/null
	then
		eval `$prefix/bin/ctxvar -ifv s_db_home_file $ctxfile`
	else
		print -u2 "$prog: cannot locate context file for $oracle_sid or $un"
		return 1
	fi

	. $s_db_home_file 
	[ -t ] && PS1=`$prefix/bin/pset -sh -T` export PS1
}
typeset -fx setdb
