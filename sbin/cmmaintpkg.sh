#!/bin/sh -
#ident $SVNHeader: TASdba/trunk/sbin/cmmaintpkg.sh 875 2014-05-20 14:21:11Z hz2rv4 $
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#
# cmmaintpkg: wrapper for cmmaintpkg command
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
PATH=/usr/sbin:/bin:/usr/bin:/usr/contrib/bin export PATH

prog=`basename $0`
cmd=/usr/contrib/lib/cmmaintpkg
usage="[-v] [-d|-e] [-n node_name]... package_name"

while getopts vedn: opt 2>&-
do
	case $opt in
	v)	if [ "$vflg" ]	# verbose
		then
			vflg="${vflg}v"
		else
			vflg=" -v"
		fi
		;;
	e)	eflg=y			# enable
		[ $dflg ] && erfflg=y
		mode=-e
		;;
	d)	dflg=y			# disable
		[ $eflg ] && erfflg=y
		mode=-d
		;;
	n)	nodes="${nodes:+$nodes }`echo \"$OPTARG\" | sed 's;,; ;g'`"
		nflg=y
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 1 ] || errflg=y
mnp="$1"

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

pkg=PRODUCT		# required SD-UX software package

if cmviewcl -v -f line -p $mnp >/dev/null 2>&1
then
	clnodes=`cmviewcl -v -f line -p $mnp 2>&- |
		awk -F\| '/^node:/ { print $2 }' |
		while read line
		do
			eval $line
			if [ -n "$name" -a "$name" != "$oldname" ]
			then
				print $name; oldname=$name
			fi
		done`
else
	cmviewcl -v -f line -p $mnp 2>&1 | 
	while read line
	do
		if expr "$line" : ".*Permission denied" >&-
		then 
			echo "$prog: user doesn't have access to view the cluster configuration" >&2
			exit 1
		fi
		echo "$line" | sed "s;^cmviewcl:;$prog:;"
	done
	exit 1
fi

if [ $nflg ]
then
	for node in $nodes
	do
		found=
		for racnode in $clnodes
		do
			if [ $node = $racnode ]
			then
				found=y
				break
			fi
		done
		if [ ! "$found" ]
		then
			echo "$prog: node '$node' not configured for package" >&2
			exit 1
		fi
	done
else
	nodes="$clnodes"
	[ "$vflg" = " -vv" ] && echo "$prog: configured nodes:" $nodes
fi

ssh="ssh -o BatchMode=yes -o ChallengeResponseAuthentication=no"

for node in $nodes
do
	if $ssh -n $node exit
	then
		:
	else
		echo "$prog: cannot connect to node '$node', check ssh settings" >&2
		exit 1
	fi
done
for node in $nodes
do
	if /usr/sbin/swlist -l product $pkg @$node >/dev/null 2>&1
	then
		:
	else
		echo "$prog: package $pkg is not installed on $node" >&2
		exit 1
	fi
done

hostname=`hostname`

# check that we have the privilege to run this command on local node only
if privrun -t $cmd 2>&-
then
	:
else
	echo "$prog: you do not have the required privileges to execute" >&2
	exit 1
fi

for node in $nodes
do
	if [ "$node" = $hostname ]
	then
		logger -t syslog -p user.info "$cmd${vflg}${mode:+ $mode} $mnp"
		privrun $cmd${vflg}${mode:+ $mode} $mnp || exit $?
	else
		$ssh $node "logger -t syslog -p user.info \"$cmd${vflg}${mode:+ $mode} $mnp\"; privrun $cmd${vflg}${mode:+ $mode} $mnp" || exit $?
	fi		
done
