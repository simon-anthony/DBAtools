divert(-1)dnl
changequote([+, +])dnl
# vim:syntax=sh:ts=4:sw=4:
divert(0)dnl
#!/bin/sh -
#ident $Header$
# WARNING:
# Clusterware is under the control of ServiceGuard Extensions for RAC (SGeRAC)
#
# You MUST NOT stop Clusterware directly without taking actions to first
# put the cluster package in "maintenance mode" on a specific node, or all 
# nodes, in the cluster.
# 
# Disregarding these instructions may cause SGeRAC to FAIL the package, stop
# Clusterware on all nodes and deactivate all shared logical volumes used 
# by Clusterware. If this is accidentally done it may be necessary to 
# re-enable node switching for the package for each node that has been failed
# with cmmodpkg(1m)
#
# This file is generated automatically. Any changes you make may go away.

ORA_CRS_HOME=__ORA_CRS_HOME__
PATH=$ORA_CRS_HOME/bin:/usr/sbin:/usr/bin:/usr/contrib/bin:__BIN__:__SBIN__
export ORA_CRS_HOME PATH

prog=`basename $0 .sh`
action=`expr "\`basename $0 .sh\`" : '\([^_]*\)_.*'`

usage() {
	cat <<!
usage: $prog [-m]      $action clusterware

   -m   Perform $action of Clusterware using package maintenance mode and crsctl.
!
}

while getopts m opt 2>&-
do
	case $opt in
	m)	mflg=y		# use maintenance mode to start/stop clusterware 
		;;
	\?)	errflg=y
	esac
done
[+shift+] `expr $OPTIND - 1`

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

if [ "$action" = start ]
then
	if [ $mflg ]
	then
		crsctl start crs -wait
		cmmaintpkg -v -d __MNP__
	else
		cmrunpkg -v __MNP__ || exit $?
	fi
fi

if [ "$action" = stop ]
then
	if [ $mflg ]
	then
		cmmaintpkg -v -e __MNP__
		crsctl stop crs -wait
	else
		cmhaltpkg -v __MNP__ || exit $?
	fi
fi

exit 0

# STATUS
# The package status is shown with the following command
	cmviewcl -p __MNP__
# Clusterware services - useful status information:
	crs_stat -t -v
	crsctl check crs
	crsctl check cluster

# LOGS
# ServiceGuard Extensions for RAC (SGeRAC) logs are found in:
#  For RAC database:
	__HOSTNAME__:/etc/cmcluster/__MNP__/__MNP__.ctl.log
#  For Clusterware:
	__HOSTNAME__:/etc/cmcluster/crsp-slvm/crsp-slvm.ctl.log
#  CRS daemon logs:
	__HOSTNAME__:$ORA_CRS_HOME/log/__HOSTNAME__/crsd
#  CRS also uses syslog:
	__HOSTNAME__:/var/adm/syslog

# NODE SWITCHING
# To re-enable node switching:
#	/usr/sbin/cmmodpkg -v -e -n <node>... <package>
__CMD_CMMODPKG__
# See cmmodpkg(1m) for more information

# MAINTENANCE MODE
# When this is enabled it is possible to shut down Clusterware without
# SGeRAC failing the package. See cmmaintpkg(1M).
#  Enable:
	cmmaintpkg -e __MNP__
#  Disable:
	cmmaintpkg -d __MNP__
#  Status:
	cmmaintpkg __MNP__

# BUGS
# After a server crash, there is a possibilty that Clusterware will loop
# indefinitely when attempting to start because of the contents of a file it
# maintains as a flag are not what is expected. In this case, execute the 
# following as the 'root' user:
	who -b > /var/opt/oracle/scls_scr/__HOSTNAME__/root/cssrun
changequote(`,)dnl
