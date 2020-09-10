changequote([+, +])dnl		# shell quotation marks need protecting
#ident $Header$
# vim:syntax=sh:ts=4:sw=4:
################################################################################
#
# Description:
#  dbshut replacement for generic shutdown script.
#  Expect ORACLE_HOME and ORACLE_SID in the environment.
#  Only acts on one database, for which the invoking user must be the owner.
#  Must only be invoked by dbstart (which sets PATH, LD_LIBRARY_PATH)
#
################################################################################

: ${ORACLE_HOME:?"must be set"}

prog=`basename $0 .sh`
usage="[-ni]"

while getopts ni opt 2>&-
do
	case $opt in
	n)	nflg=y		
		run=echo
		;;
	i)	iflg=y	# HP-UX ignore SGeRAC Toolkit
		;;
	\?)	errflg=y
	esac
done
[+shift+] `expr $OPTIND - 1`

if [ $errflg ]
then
	echo usage: $prog $usage >&2
	exit 2
fi

unset TWO_TASK

# pfile not used in shutdown -  it's here just to check ORACLE_SID
pfile=${ORACLE_HOME}/dbs/init${ORACLE_SID:?"must be set"}.ora

if [ `uname -s` = "HP-UX" -a -z "$iflg" ]
then
	if mnp=`BINDIR/racp -s ${ORACLE_GLNAM:-$ORACLE_SID}`
	then
		[ $nflg ] && echo "# ${ORACLE_GLNAM:-$ORACLE_SID} is under control of SGeRAC package $mnp"
		$run /usr/sbin/cmhaltpkg -v $mnp
		exit
	fi
fi

SBINDIR/dbacmd ${run:+-n} ${ORACLE_TRACE:+-d} shutdown $*
retval=$?

if [ $retval -eq 0 ]
then
	[ $run ] || echo Database \"${ORACLE_SID}\" shut down.
else
	[ $run ] || echo Database \"${ORACLE_SID}\" not shut down.
fi
exit $retval
changequote(`,)dnl
