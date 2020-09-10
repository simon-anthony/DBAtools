#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:

PATH=/bin:/usr/bin:BINDIR export PATH
FPATH=BASEDIR/share/lib/functions export FPATH

prog=`basename $0`
usage="[-k skel_dir] -s <SID>"

unset ORACLE_SID

while getopts adfs:k: opt 2>&-
do
	case $opt in
	f)	fflg=y						# force
		;;
	k)	kflg=y						# alternative skeleton directory
		skel_dir="$OPTARG"
		;;
	s)	ORACLE_SID=$OPTARG export ORACLE_SID
		sflg=y						# SID
		unset TWO_TASK
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 0 ] || errflg=y
username=`id -un`
if [ $username = oracrs ]
then
	subdir=crs
else
	subdir=oracle
	if [ $kflg ]
	then
		[ -r "$skel_dir" ] || { echo $prog: cannot read skel_dir >&2; exit 1; }
	fi
	if [ ! "$sflg" ]
	then
		errflg=y
	else
		if BINDIR/adstatus -g off -n -s $ORACLE_SID > /tmp/$prog$$ 2>&1
		then
			:
		else
			echo ERROR: invalid ORACLE_SID
			exit 1
		fi
	fi
fi
[ $kflg ] || skel_dir=SYSCONFDIR/$subdir/skel
	 
[ $errflg ] && { echo usage: $prog $usage >&2; exit 2; }

for file in `ls -a $skel_dir 2>/dev/null`
do
	[ -f $skel_dir/$file ] || continue
	echo $prog: installing $file
	[ -f $HOME/$file -a -z "$fflg" ] && preserve $HOME/$file
	rm -f $HOME/$file
	sed "s;%s_dbSid%;$ORACLE_SID;g
		 s;%sbindir%;SBINDIR;g
		 s;%bindir%;BINDIR;g
		 s;%datadir%;DATADIR;g
		 s;%basedir%;BASEDIR;g" $skel_dir/$file > $HOME/$file
	chmod 600 $HOME/$file
done
