divert(-1)dnl
changequote([+, +])dnl
# vim:syntax=sh:ts=4:sw=4:
divert(0)dnl
#!/bin/sh -
#ident $Header$
# WARNING:
# This database is under the control of ServiceGuard Extensions for RAC (SGeRAC)
#
# You MUST NOT shut the database down directly (with sqlplus, srvctl or any 
# other method) without taking actions to first put the cluster package in
# "maintenance mode" on a specific node, or all nodes, in the cluster.
# 
# Disregarding these instructions may cause SGeRAC to FAIL the package, stop
# the entire database and deactivate all shared logical volumes used by the
# database. If this is accidentally done it may be necessary to re-enable node
# switching for the package for each node that has been failed with cmmodpkg(1m)
#
# This file is generated automatically. Any changes you make may go away.

ORA_CRS_HOME=__ORA_CRS_HOME__
PATH=$ORA_CRS_HOME/bin:/usr/sbin:/usr/bin:/usr/contrib/bin:__BIN__:__SBIN__
export ORA_CRS_HOME PATH

prog=`basename $0 .sh`
action=`expr "\`basename $0 .sh\`" : '\([^_]*\)_.*'`

usage() {
	cat <<!
usage: $prog [-m] [-r] $action database and apps tiers in cluster
       $prog -d [-m]   $action only database tier
       $prog -a [-r]   $action only apps tier

   -m   Perform $action of database using package maintenance mode and srvctl.

   -r   Include remote applications tiers in $action.
!
}

while getopts cdmar opt 2>&-
do
	case $opt in
	d)	dflg=y		# start/stop only database
		[ $aflg ] && errflg=y
		;;
	m)	mflg=y		# use maintenance mode to start/stop database 
		;;
	a)	aflg=y		# start/stop only apps tiers
		[ $dflg ] && errflg=y
		;;
	r)	rflg=y		# start/stop apps services on remote nodes
		;;
	\?)	errflg=y
	esac
done
[+shift+] `expr $OPTIND - 1`

[ "$rflg" -a "$dflg" ] && errflg=y

[ $errflg ] && { usage >&2; exit 2; }

if [ $mflg ]
then
	# Package state 
	[+eval+] `cmviewcl -f line -p __MNP__ | awk '/^state=/ { print }'`

	if [ "$state" != running ]
	then
		echo Package __MNP__ must be running for maintenance mode >&2
		exit 1
	fi
fi

if [ "$action" = start -a ! "$aflg" ]
then
__STARTSTOP_LISTENERS__
	if [ $mflg ]
	then
		srvctl $action database -d __SID__
		cmmaintpkg -v -d __MNP__
	else
		if cmmaintpkg __MNP__ | grep -q ENABLED
		then
			echo Package __MNP__ is currently in maintenance mode >&2
			echo Start database with srvctl or use -m option >&2
			exit 1
		fi
		cmrunpkg -v __MNP__ || exit $?
	fi
fi

if [ ! "$dflg" ]
then
__STARTSTOP_SERVICES__
	if [ "$rflg" ]
	then
__STARTSTOP_REMOTE_SERVICES__
	fi
fi

if [ "$action" = stop -a ! "$aflg" ]
then
	if [ $mflg ]
	then
		cmmaintpkg -v -e __MNP__
		srvctl $action database -d __SID__ -o immediate
	else
		cmhaltpkg -v __MNP__ || exit $?
	fi
__STARTSTOP_LISTENERS__
fi

exit 0

# STATUS
# The package status is shown with the following command:
	cmviewcl -p __MNP__
# Clusterware services - useful status information:
    crsctl check crs
	crsctl check cluster
	crs_stat -t -v
# Configuration of package can be obtained thus:
	cmgetconf -p __MNP__

# LOGS
# ServiceGuard Extensions for RAC (SGeRAC) logs are found in:
#  For Clusterware:
	__HOSTNAME__:/etc/cmcluster/crsp-slvm/crsp-slvm.ctl.log
#  SGeRAC also logs to syslog:
	__HOSTNAME__:/var/opt/syslog/*
# CRS daemon logs:
	__HOSTNAME__:$ORA_CRS_HOME/log/__HOSTNAME__/crsd/*
# RAC/srvctl logs:
	__HOSTNAME__:$ORACLE_HOME/log/__HOSTNAME__/racg/*

# NODE SWITCHING
# To re-enable node switching:
#	/usr/sbin/cmmodpkg -v -e -n <node>... <package>
__CMD_CMMODPKG__
# See cmmodpkg(1m) for more information

# MAINTENANCE MODE
# When this is enabled it is possible to shut down an instance without
# SGeRAC failing the package. See cmmaintpkg(1M).
#  ENABLE:
#  For example, for the entire database:
	cmmaintpkg -e __MNP__
	srvctl stop database -d __SID__ -o immediate
#  Or, for a single node and instance:
	cmmaintpkg -e -n __HOSTNAME__ __MNP__ 
	srvctl stop instance -d __SID__ -i __INSTANCE__ -o immediate
#  DISABLE:
	srvctl start database -d __SID__ 
	cmmaintpkg -d __MNP__
#  STATUS:
	cmmaintpkg __MNP__
# NB. If Clusterware is in maintenance mode all dependent SGeRAC database
# packages are implicitly in maintenance mode.
changequote(`,)dnl
