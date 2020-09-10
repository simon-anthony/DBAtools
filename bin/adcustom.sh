#!/bin/ksh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#
# Check custom templates
#
################################################################################
PATH=/bin:/usr/bin:BINDIR export PATH
FPATH=BASEDIR/share/lib/functions export FPATH

prog=`basename $0`
usage="[-y] [-n] {-a <path to apps context file>|-s <s_systemname>}"

while getopts a:s:yn opt 2>&-
do
	case $opt in
	a)	appctxfile="$OPTARG"
		[ $sflg ] && errflg=y
		aflg=y
		;;
	s)	s_systemname="$OPTARG"
		[ $aflg ] && errflg=y
		sflg=y
		;;
	y)	yflg=y
		;;
	n)	nflg=y
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ ! "$aflg" -a ! "$sflg" ] && errflg=y
[ $# -ne 0 ] && errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

# Context file(s)
#
if [ $aflg ]
then
	appctxfile=`getctx -a -f $appctxfile` || exit $?
else
	appctxfile=`getctx -a -s $s_systemname` || exit $?
fi
echo $prog: using apps context file $appctxfile

if [ "$sflg" -a ! "$yflg" ]
then
	ans=`ckyorn -d y -p "Continue?"` || exit $?
	case $ans in
	y) : ;;
	*) exit
	esac
fi
################################################################################
# Get AD_TOP, FND_TOP
for var in s_adtop s_fndtop
do
	eval $var=`ctxvar -if $var $appctxfile`
done

version() {
	ident $1 2> /dev/null | awk '
        /Header:/ { printf("%s", $3); exit; }'
}
chkcust() {
	[ -d $1 ] || return 1
	echo Checking $1
	cd $1/.. || return 1
	n=0
	for file in *
	do
		expr "$file" : '.*\.tmp' >&- && continue	# don't do tmp files
		if [ -f $1/$file ]
		then
			# customized version exists
			[ n -eq 0 ] && \
				printf "%-42s %9s %9s %17s\n" Filename Delivered Custom "Update Required?"
			v=`version $file`
			cv=`version $1/$file`
			[ "$v" != "$cv" ] && update=Yes || update=
			printf "%-42s %9s %9s %9s\n" $file $v $cv $update
			(( n += 1 ))
		fi
	done
	echo
}

# DB Tier 
chkcust $s_adtop/admin/template/install/template/db/custom

# Apps Tier
chkcust $s_adtop/admin/template/custom
chkcust $s_fndtop/admin/template/custom

