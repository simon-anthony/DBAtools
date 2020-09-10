#!/bin/ksh -
#ident $Header$
################################################################################
# Instantiate custom AutoConfig templates given a context file
#
################################################################################
PATH=/bin:/usr/bin:BINDIR:BASEDIR/bin export PATH
FPATH=BASEDIR/share/lib/functions export FPATH

prog=`basename $0`
usage() {
cat <<!
usage: $prog [-v <var>=<val>...] -f <ctxfile> [<file>]
!
[ $help ] && cat <<!

   -v <var>=<val>
             specify Autoconfig variable value pairs which take precedence
             over context file

   -f <ctxfile>
             specify a database or applications context file

   <file>
             is the name of the template to instantiate
!
}

while getopts f:yv: opt 2>&-
do
	case $opt in
	f)	ctxfile="$OPTARG"
		fflg=y
		;;
	v)	vals="-v $vals $OPTARG"
		vflg=y
		;;
	\?)	errflg=y
		help=y
	esac
done
shift `expr $OPTIND - 1`

[ "$fflg" ] || errflg=y

[ $# -gt 1 ] && errflg=y

[ $# -eq 1 ] && tmpl=$1

[ $errflg ] && { usage >&2; exit 2; }

# Context file
#
[ -r $ctxfile ] || { print -u2 "$prog: cannot read $ctxfile"; exit 1; }

eval `ctxvar -ifv s_contexttype $ctxfile`

case "$s_contexttype" in
"Database Context")	type=db ;;
"APPL_TOP Context") type=apps ;;
*) type=error ;;
esac

if [ "$type" = "error" ]
then
	print -u2 "$prog: invalid context file"
	exit 1
fi

if [ "$type" = "db" ]
then
	eval `ctxvar -ifv s_db_oh $ctxfile`
	s_adtop=
	# the installed custom directory for the db - there is no point in
	# instantiating in AD_TOP...
	tmpldir=$s_db_oh/appsutil/template
	custom=$tmpldir/custom
	drv=$tmpldir/addbtmpl.drv
elif [ "$type" = "apps" ]
then
	eval `ctxvar -ifv s_adtop $ctxfile`
	tmpldir=$s_adtop/admin/template
	custom=$tmpldir/custom
	drv=$s_adtop/admin/driver/adtmpl.drv
fi

if [ "X$tmpl" != "X" ]
then
	if [ ! -r $custom/$tmpl ]
	then
		print -u2 "no custom template for '$tmpl' in '$custom'"
		ls -1 $custom
		exit 1
	fi
fi

printf "drv is %s\n\n" $drv

for tmpl in `ls $custom 2>&-`
do
	set -- `awk '$3 == "'$tmpl'" { print }' $drv`

	[ $# -eq 0 ] && continue # no entry in driver file

	# Driver entries:
	#  <src_top> <src_dir> <src_name> <file_type> <dst_dir> <dst_name> [<mode>]
	#
	src_top=$1 src_dir=$2 src_name=$3 file_type=$4 dst_dir=$5 dst_name=$6
	mode=$7

	#  If no context variable starts the value of <src_dir> then it is
	#  relative to <src_top>
	#
	if expr "$src_dir" : "[^<].*" >&-
	then
		rel_dir=`ctxvar -if s_${src_top}top $ctxfile`
	else
		rel_dir=
	fi
		
	src=`echo ${rel_dir:+$rel_dir/}$src_dir/custom | suboavars -d -f $ctxfile`
	dst=`echo $dst_dir | suboavars -d -f $ctxfile $vals`

	printf "inst %s/%s\n     %s/%s\n\n" \
		"${rel_dir:+<s_${src_top}top>/}$src_dir/custom" "$src_name" \
		"$dst_dir" "$dst_name" 
	printf "     %s/%s\n     %s/%s\n\n" \
		"$src" "$src_name" \
		"$dst" "$dst_name" 

	suboavars -f $ctxfile < $src/$src_name > $dst/$dst_name
	[ "$mode" ] && chmod $mode $dst/$dst_name
done
