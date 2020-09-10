#!/bin/sh -
#ident $SVNHeader: TASdba/trunk/sbin/backup_example.sh 875 2014-05-20 14:21:11Z hz2rv4 $
# vim:syntax=sh:sw=4:ts=4:
# 
PATH=/usr/sbin:/usr/bin:/usr/contrib/bin export PATH

prog=`basename $0`
usage="[-f] package_name"

while getopts f opt 2>&-
do
	case $opt in
	f)	fflg=y			# force halt of database package
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 1 ] || errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

package_name="$1"

if eval `cmviewcl -f line -p $package_name`
then
	: # the variables: name= type= status= state= autorun= are now set
else
	echo $prog: failed to determine status of package $package_name >&2
	exit 1
fi

if [ $state = "running" ]
then
	if [ $fflg ]	# force shut down
	then
		cmhaltpkg $name
	else
		echo $prog: package $name is $state, unable to backup >&2; exit 1
	fi
fi

if [ $state != "halted" ]
then
	echo $prog: package $name is $state, unable to backup >&2; exit 1
fi

cmvgpkg -a $name        # activate volume groups

# do backup ...

