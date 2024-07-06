#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
PATH=/bin:/usr/bin:BINDIR export PATH
FPATH=BASEDIR/share/lib/functions export FPATH

prog=`basename $0`
usage="-s <dbname>"

while getopts s: opt 2>&-
do
	case $opt in
	s)	dbname="$OPTARG"
		sflg=y
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $sflg ] || errflg=y
[ $# -ne 0 ] && errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

eval `grep '^ORA_CRS_HOME=' /etc/cmcluster/*/oc.conf`	# set the last found
if [ "$ORA_CRS_HOME" ] 
then
	export ORA_CRS_HOME
else
	echo ERROR: cannot locate ORA_CRS_HOME >&2; exit 1
fi

. adenv -do $dbname
instname=$ORACLE_SID
PATH=$ORA_CRS_HOME/bin:$PATH

hostname=`hostname`

[ "$so" ] || { [ -t ] && so=`tput smso` off=`tput sgr0`; }
[ "$bold" ] || { [ -t ] && bold=`tput bold` off=`tput sgr0`; }

header() {
	print "\n$bold$*$off"
}
ul() {
	print "$so$*$off"
}
# 844272.1

# check 1 - srvctl start command is received by the crsd daemon
header "Checking last attempt to start instance ..."
print "`date '+%Y-%m-%d %X.000:'` Current date and time"
sed -n '
/Attempting to start `ora\.'$dbname'\.'$instname'\.inst` on member `'$hostname'`/ {
	h; }
$ { x; s;.*;'$so'&'$off';; p; }'  \
$ORA_CRS_HOME/log/$hostname/crsd/crsd.log

# check 2 - instance status is OFFLINE before starting it
header "Checking instance status..."

state() {
	crs_stat -f $1 | sed -n '/^STATE=/ s;.*=\([^ 	]\{1,\}\).*;\1;p'
}

BINDIR/crsstat | awk '
	/^Name/ { print }
	/^----/ { print }
	$1 ~ /ora\.'$dbname'\.'$instname'\.inst/ {
		printf("'$so'%s'$off'\n", $0)
	}'
if [ `state ora.$dbname.$instname.inst` = OFFLINE ]
then
	print "Instance state is OK to attempt restart using:"
	print "\tsrvctl start instance -d $dbname -i $instname"
	ans=`ckyorn -p "Attempt start?"` || exit $?
	case $ans in
	[yY]*)  srvctl start instance -d $dbname -i $instname ;;
	*)      exit $?
	esac
elif [ `state ora.$dbname.$instname.inst` = UNKNOWN ]
then
	print "Instance State is UNKNOWN - manual intervention required."
	print "Try:"
	print "\tsrvctl stop instance -d $dbname -i $instname"
	print "to return the state to OFFLINE"
	ans=`ckyorn -p "Attempt stop?"` || exit $?
	case $ans in
	[yY]*)  srvctl stop instance -d $dbname -i $instname ;;
	*)      exit $?
	esac
	if [ `state ora.$dbname.$instname.inst` = UNKNOWN ]
	then
		print "Instance State is still UNKNOWN."
		print "Try:"
		print "\tcrs_stop -f ora.$dbname.$instname.inst"
		print "to return the state to OFFLINE"
		ans=`ckyorn -p "Attempt stop?"` || exit $?
		case $ans in
		[yY]*)	crs_stop -f ora.$dbname.$instname.inst ;;
		*)		exit $?
		esac
	fi
	if [ `state ora.$dbname.$instname.inst` = UNKNOWN ]
	then
		print "Instance State remains UNKNOWN. Options exhausted"
		exit 1
	fi
elif [ `state ora.$dbname.$instname.inst` = ONLINE ]
then
	print "Instance State is ONLINE - shutdown and retry test"
fi

racglogs -d $dbname -i $instname start

# check 4
srvctl config database -d $dbname -a
srvctl getenv database -d $dbname
srvctl getenv instance -d $dbname -i $instname

