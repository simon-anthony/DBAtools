#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
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
PATH=/usr/bin
export PATH 

prog=`basename $0`
usage="[-n] [-q] [-f <file>]"
 
while getopts nqf: opt 2>&-
do
	case $opt in
	n)	nflg=y
		;;
	q)	qflg=y
		;;
	s)	sflg=y	# strict host key checking
		shkc=yes
		;;
	f)	fflg=y
		if [ -r "$OPTARG" ] 
		then
			exec < "$OPTARG"
		else
			echo $prog: cannot read \'$OPTARG\' >&2
			exit 1
		fi
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`
 
[ $errflg ] && { echo usage: $prog $usage >&2; exit 2; }

if [ -t ]
then
	bold=`tput bold`
	off=`tput sgr0`
fi

echob() {
	echo $bold$*$off
}

log=`mktemp`

# Test ssh connection(s)...
ssh="ssh -o BatchMode=yes
-o ChallengeResponseAuthentication=no
-o StrictHostKeyChecking=${shkc:=no}
"

host=
rcmd=

host=`hostname`
user=`id -un`

while read local remote
do
	if expr "$local" : '[ 	]*#.*' >/dev/null
	then
		continue
	fi
	if [ "$remote" ]
	then
		:
	else
		remote="$local"
		local=$host
	fi
	lhost=${local##*@}			# host part
	if [ ${local%%@*} != $local ]
	then
		luser=${local%%@*}			# user part
	fi

	rhost=${remote##*@}			# host part
	if [ ${remote%%@*} != $remote ]
	then
		ruser=${remote%%@*}			# user part
	fi

	echo "============================================"
	#printf "%-17s --> %-s\n" \
	#	${luser:-$user}@${lhost:-$host} ${ruser:-$user}@$rhost
	if [ "$host" = "$lhost" -a "$user" = "${luser:-$user}" ]
	then
		:
		#echo "$prog: testing connection to $host... \c"
		#echo rcmd="ssh -n ${luser:-$user}@$rhost"
		#rcmd="ssh -n ${ruser:-$user}@$rhost"
		rcmd="\$ssh -n ${ruser:-$user}@$rhost"
		echob $rcmd
		if eval $rcmd exit ${qflg:+">$log 2>&1"}
		then
			echo OK
		else
			echo Failed
		fi
	else
		#echo "$prog: testing reverse connection from $host to $lhost... \c"
		#echo rrcmd="ssh -n $luser@$lhost"
		#rrcmd="ssh -n $luser@$lhost"
		rcmd="\$ssh -n ${luser:-$user}@$lhost"
		rrcmd="\$ssh -n ${ruser:-${luser:-$user}}@$rhost"
		echob $rcmd \"$rrcmd\"
		if eval $rcmd exit ${qflg:+">$log 2>&1"}
		then
			if eval $rcmd "$rrcmd exit" ${qflg:+">$log 2>&1"}
			then
				echo OK
			else
				echo Failed
			fi
		else
			echo Failed \($rcmd\)
		fi
	fi
	unset luser ruser lhost rhost local remote
	continue
 
	echo "$prog: testing connection to $host... \c"
	if ssh -n $host exit
	then
		echo OK
		rcmd="ssh -n $host"
	else
		echo Failed
		continue
	fi
	echo "$prog: testing reverse connection from $host to $localhost... \c"
	if $rcmd "ssh -n $localuser@$localhost exit"
	then
		echo OK
		rrcmd="ssh -n $localuser@$localhost"
	else
		echo Failed
	fi
done
