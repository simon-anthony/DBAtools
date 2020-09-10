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

prog=`basename $0`
action=`expr "\`basename $0 .sh\`" : '\([^_]*\)_.*'`

usage="[-v]\n   -v   verbose"

while getopts v opt 2>&-
do
	case $opt in
	v)	vflg=y		# verbose
		;;
	\?)	errflg=y
	esac
done
[+shift+] `expr $OPTIND - 1`

[ $errflg ] && { echo "usage: `basename $0` $usage" >&2; exit 2; }

# STATUS
# The package status is shown with the following command:
	cmviewcl ${vflg:+-v} -p __MNP__
# Clusterware services - useful status information:
[+#+]	srvctl status database -d __SID__
__LIST_STATUS_INSTANCE__
# Status of maintenance mode:
	cmmaintpkg ${vflg:+-v} __MNP__
if [ $vflg ]
then
__LIST_STATUS_NODEAPPS__
	# Show the full name of the resource using this (crs_stat -t truncates this)
	crs_stat | awk -F= '
		BEGIN {	
			printf("%-30s%-16s%-9s%-9s%-9s\n",
				"Name", "Type", "Target", "State", "Node")
			for (i=0; i<73; i++)	
				printf("-")
			printf("\n")
		}
		/^$/ {
			printf("\n"); next
		}
		$1 == "NAME" {
			printf("%-30s", $2); next
		}
		$1 == "TYPE" {
			printf("%-16s", $2); next
		}
		$1 == "TARGET" {
			printf("%-9s", $2); next
		}
		$1 == "STATE" {
			if ($2 ~ /^ONLINE/) {
				printf("%-9s", [+substr+]($2, 1, [+index+]($2, " ")))
				printf("%-9s", [+substr+]($2, [+index+]($2, "on ")+3))
			} else
				printf("%-9s", $2)
			next
		}'
fi
changequote(`,)dnl
