changequote([+, +])dnl		# shell quotation marks need protecting
#!/usr/bin/ksh 
# vim:syntax=sh:ts=4:sw=4:
################################################################################
#
# Description:
#  dbstart replacement for generic startup script.
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

pfile=${ORACLE_HOME}/dbs/init${ORACLE_SID:?"must be set"}.ora
spfile=${ORACLE_HOME}/dbs/spfile$ORACLE_SID.ora

if [ `uname -s` = "HP-UX" -a -z "$iflg" ]
then
	if mnp=`BINDIR/racp -s ${ORACLE_GLNAM:-$ORACLE_SID}`
	then
		[ $nflg ] && echo "# ${ORACLE_GLNAM:-$ORACLE_SID} is under control of SGeRAC package $mnp"
		$run /usr/sbin/cmrunpkg -v $mnp
		exit
	fi
fi

started=
if [ -f $ORACLE_HOME/dbs/sgadef$ORACLE_SID.dbf -o \
	  -f $ORACLE_HOME/dbs/sgadef$ORACLE_SID.ora ]
then
	started=y
fi

if ps -ef | grep pmon_"$ORACLE_SID\>" | grep -vc grep >/dev/null
then
	echo Database \"$ORACLE_SID\" already started.
	started=y
fi

if [ $started ]
then
	[ $run ] || echo Database \"$ORACLE_SID\" possibly left running when system went down \(system crash?\).
	[ $run ] || echo Notify Database Administrator.
	SBINDIR/dbacmd ${run:+-n} ${ORACLE_TRACE:+-d} shutdown abort
fi

if [ -f $pfile -o -f $spfile ]
then
	SBINDIR/dbacmd ${run:+-n} ${ORACLE_TRACE:+-d} startup $*
	retval=$?

	if [ $retval -eq 0 ]
	then
		[ $run ] || echo Database \"$ORACLE_SID\" started.
	else
		[ $run ] || echo Database \"$ORACLE_SID\" NOT started.
	fi
else
	[ $run ] || echo Can\'t find init file for Database \"$ORACLE_SID\".
	[ $run ] || echo Database \"$ORACLE_SID\" NOT started.
fi
exit $retval
changequote(`,)dnl
