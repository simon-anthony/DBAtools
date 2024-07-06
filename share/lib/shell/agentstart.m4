changequote([+, +])dnl		# shell quotation marks need protecting
#!/usr/bin/ksh 
# vim:syntax=sh:ts=4:sw=4:
################################################################################
#
# Description:
#  Expect ORACLE_HOME and ORACLE_SID in the environment.
#  Only acts on one database, for which the invoking user must be the owner.
#  Must only be invoked by dbstart (which sets PATH, LD_LIBRARY_PATH)
#
################################################################################

: ${ORACLE_HOME:?"must be set"}

prog=`basename $0 .sh`
usage="[-n]"

while getopts n opt 2>&-
do
	case $opt in
	n)	nflg=y		
		run=echo
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

if [ $prog = agentstart ]
then
	action=start
elif [ $prog = agentstop ]
then
	action=stop
else
	action=status
fi

unset TWO_TASK

: ${ORACLE_SID:?"must be set"}

conf=${ORACLE_HOME}/sysman/config/emd.properties

[ -r $conf ] || {
	[ $nflg ] && echo $ORACLE_SID: not a configured agent home >&2
	exit 1
}

id=`expr "\`id\`" : '[^(]\{1,\}(\([^ ]\{1,\}\))'`   # non XPG4 id

if [ $id = root ]
then
	owner=`ls -l $ORACLE_HOME/bin/kgpmon | cut -d\  -f5`
fi

PATH=${ORACLE_HOME}/bin:$PATH export PATH

unset ORA_NLS33		# See note: 304461.1

name=agent

echo "$ORACLE_SID: \c"
[+eval+] $run ${owner:+"su $owner -c '"}emctl $action $name${owner:+"'"}
changequote(`,)dnl
