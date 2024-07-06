#!/usr/bin/ksh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#
# Shortcut to run adbatch -ga
#
################################################################################
PATH=/bin:/usr/bin:BINDIR:SBINDIR export PATH
FPATH=BASEDIR/share/lib/functions export FPATH

prog=`basename $0`
usage() {
	cat <<!
usage: $prog -p <password> -s <sid>
!
[ $help ] && cat <<!

   -p        Password for the SYSTEM oracle database account

   -s <sid>  Use <sid> to determine the environment

!
}

while getopts s:p: opt 2>&-
do
	case $opt in
	s)	sid="$OPTARG"
		sflg=y
		;;
	p)	password="$OPTARG"
		pflg=y
		;;
	y)	yflg=y
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $pflg ] || errflg=y
[ $sflg ] || errflg=y
[ $# -ne 0 ] && errflg=y

[ $errflg ] && { usage >&2; exit 2; }

. adenv $sid || exit $?

if oiconnect system/$password
then
	:
else
	echo $prog: password for user SYSTEM incorrect >&2
	exit 1
fi

if appspassword=`getappspass -f -s $sid 2>&-`
then
	:
else
	echo $prog: could not determine APPS password >&2
	exit 1
fi

adbatch -gfa |&

# Note that because there are no newlines after the questions the answers
# lag one behind
while read -rp line
do
	print "$line"
	case "$line" in
	"Your default directory is"*)
	#"Is this the correct APPL_TOP"*)
		print -p "yes"
		continue;;
	"Is this the correct APPL_TOP"*)
	#"Filename"*)
		print -p ""
		continue;;
	"Filename"*)
	#"Do you wish to activate this feature"*)
		print -p ""
		continue;;
	"Do you wish to activate this feature"*)
	#"Please enter the batchsize"*)
		print -p ""
		continue;;
	"Please enter the batchsize"*)
	#"Is this the correct database"*)
		print -p "yes"
		continue;;
	"Is this the correct database"*)
	#"Enter the password for your 'SYSTEM' ORACLE schema"*)
		print -p "$password"
		continue;;
	"Enter the password for your 'SYSTEM' ORACLE schema"*)
	#"Enter the ORACLE password of Application Object Library"*)
		print -p "$appspassword"
		continue;;
	"Enter the ORACLE password of Application Object Library"*)
	#"The default directory is"*)
		print -p "abort"
		continue;;
	esac
done

print "$APPL_TOP/admin/$TWO_TASK/def.txt created"

exit 0
