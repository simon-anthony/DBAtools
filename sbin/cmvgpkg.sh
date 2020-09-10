#!/bin/sh -
#ident $SVNHeader: TASdba/trunk/sbin/cmvgpkg.sh 875 2014-05-20 14:21:11Z hz2rv4 $
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#
# cmvgpkg: wrapper for cmvgpkg command
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
cmd=/usr/contrib/lib/cmvgpkg
usage="[-v] [-a[-s]|[-d[-s]] [-n node_name]... package_name"
if [ "A$CMVGPKG_LOCAL_ONLY" != "A" -a "A$CMVGPKG_LOCAL_ONLY" != "A0" ]
then
	usage="[-v] [-a[-s]|[-d[-s]] package_name"
fi

while getopts vasdn: opt 2>&-
do
	case $opt in
	v)	if [ "$vflg" ]	# verbose
		then
			vflg="${vflg}v"
		else
			vflg=" -v"
		fi
		;;
	a)	aflg=y			# activate
		if [ $dflg ]
		then
			errflg=y
		fi
		mode=-a
		;;
	d)	dflg=y			# deactivate
		if [ $aflg ]
		then
			errflg=y
		fi
		mode=-d
		;;
	s)	sflg=y			# produce script
		;;
	n)	nodes="${nodes:+$nodes }`echo \"$OPTARG\" | sed 's;,; ;g'`"
		nflg=y
		if [ "A$CMVGPKG_LOCAL_ONLY" != "A" -a "A$CMVGPKG_LOCAL_ONLY" != "A0" ]
		then
			errflg=y	# syntax error for old mode
		fi
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ -n "$sflg" -a ! \( -n "$aflg" -o -n "$dflg" \) ] && errflg=y	# -a or -d mandatory for -s

[ $# -eq 1 ] || errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

mnp="$1"
hostname=`hostname`

if [ "A$CMVGPKG_LOCAL_ONLY" != "A" -a "A$CMVGPKG_LOCAL_ONLY" != "A0" ]
then
	nodes=$hostname
	nflg=y
fi

if [ $sflg ]
then
	if [ $nflg ]
	then
		if [ "A$nodes" != "A$hostname" ]
		then
			echo $prog: can only specify current node when -s is used >&2;
			exit 2
		fi
	else
		nodes=$hostname
	fi
fi

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
	if [ "$node" = $hostname ]	# dont ssh to local node
	then
		continue
	fi
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

# check that we have the privilege to run this command on local node only
if privrun -t $cmd 2>&-
then
	:
else
	echo "$prog: you do not have the required privileges to execute" >&2
	exit 1
fi

# rpt: output filter for reports
#
rpt() {
	if [ -z "$nflg" -o \( -n "$nflg" -a "A${nodes% *}" != "A$nodes" \) ] 
	then
		printf "NODE_NAME\n$node\n%-32s %-26s %-10s\n" \
			"VG_NAME" "VG_STATUS" "VG_WRITE_ACCESS"
	fi
	awk '
		/^VG Name/			{ dev = $(NF) }
		/^VG Write Access/	{ rw = $(NF) }
		/^VG Status/		{ gsub(/^VG Status */, "", $0); status = $0 }
		/Volume group not activated/ {
			status = "not activated"
		}
		/Cannot display volume group/ {
			dev = substr($0, match($0, /\"/)+1,
				length(substr($0, match($0, /\"/)))-3)
			if (!match(dev, /^\/dev\//))	# prefix "/dev/" 
				dev = "/dev/" dev
			display(dev, status, rw)
		}
		/^$/ { display(dev, status, rw)  }
		/^'$prog:'/ { print | "cat >&2"; n = 1; } 
		function display(d, s, r)
		{
			printf("%-32s %-26s %-10s\n", d, s, r)
		}
		END { exit n }'
}

for node in $nodes
do
	if [ "$node" = $hostname ]
	then
		logger -t syslog -p user.info "$cmd${vflg}${mode:+ $mode}${sflg:+ -s} $mnp"

		if [ "$aflg" -o "$dflg" ]
		then
			privrun $cmd${vflg}${mode:+ $mode}${sflg:+ -s} $mnp || exit $?
		else
			privrun $cmd${vflg} $mnp 2>&1 | rpt 
		fi
	else
		$ssh $node "logger -t syslog -p user.info \"$cmd${vflg}${mode:+ $mode}${sflg:+ -s} $mnp\""

		if [ "$aflg" -o "$dflg" ]
		then
			$ssh $node "privrun $cmd${vflg}${mode:+ $mode}${sflg:+ -s} $mnp" || exit $?
		else
			$ssh $node "privrun $cmd${vflg} $mnp 2>&1" | rpt
		fi
	fi
done
echo
