#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
# ckinv: check permissions on Oracle software inventory and generate script
# to reset
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
PATH=/bin:BINDIR:SBINDIR
export PATH

usage="[-u <user>] -g <group>"
prog=`basename $0`
#group=dba
invdir=/var/opt/oracle/oraInventory

suggestgroup() {
	if [ -d $invdir ]
	then
		group=`ls -ld $invdir | awk '{ print $4 }'`
		echo $prog: suggest \"$prog -g $group\" >&2
	fi
}

while getopts u:g: opt 2>/dev/null
do
		case $opt in
		u)	user=$OPTARG
			uflg=y
			;;
		g)	group=$OPTARG
			if getgrname $group >/dev/null 2>&1 
			then
				:
			else
				echo $prog: invalid group \"$group\"
				suggestgroup
				exit 1
			fi
			gflg=y
			;;
		\?)	errflg=y
		esac
done
shift `expr $OPTIND - 1`

[ $gflg ] || errflg=y
[ $uflg ] || user=`id -un`
sgroups=`id -Gn`

[ $errflg ] && {
	echo usage: $prog $usage >&2;
	if [ ! "$gflg" ]
	then
		suggestgroup
	fi
	exit 2; }

find /var/opt/oracle/oraInventory \
	! -user $user ! -perm -0020 ! -perm 0002 \
	! \( -name \*.log\* -o -name \*.err\* -o -name \*.out\* -o -name \*.lst\* \) |
xargs -L1 ls -ld | sort -k3 | awk -v sgroups="$sgroups" '
BEGIN {
	split(sgroups, groups)
}
user != $3 {
	if (length(user))
		printf("\"\n")
	if (system("/bin/test -t") != 0)
		printf("echo User: %s\n", $3);

	if ($4 != "'$group'" && !groupmember($4))
		printf("su %s -c \"chmod o+w %s", $3, $(NF))
	else if ($4 != "'$group'" && groupmember($4))
		printf("su %s -c \"chmod g+w %s", $3, $(NF))
	else
		printf("su %s -c \"chmod g+w %s", $3, $(NF))
	user = $3
}
user == $3 {
	printf(" %s", $(NF))
}
END {
	if (length(user))
		printf("\"\n")
}
function groupmember(group)
{
	for (i in groups)
		if (groups[i] == group)
			return 1
	return 0
}'
