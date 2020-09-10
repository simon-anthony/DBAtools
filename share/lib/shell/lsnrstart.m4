changequote([+, +])dnl		# shell quotation marks need protecting
#ident $Header$
# vim:syntax=sh:ts=4:sw=4:
################################################################################
#
# Description:
#  Start/Stop listeners for this database if either:
#   A startup/shutdown script exists: addlnctl.sh
#   The file listener.ora is found in $ORACLE_HOME/network/admin
#  Expect ORACLE_HOME and ORACLE_SID in the environment.
#  Only acts on one database, for which the invoking user must be the owner.
#  Must only be invoked by lsnrstart
#
################################################################################

: ${ORACLE_HOME:?"must be set"}

if [ `basename $SHELL` = ksh ]
then
	FPATH=DATADIR/lib/functions export FPATH
else
	for file in DATADIR/lib/functions/*
	do
		[ -r $file ] && . $file
	done
fi

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

if [ $prog = lsnrstart ]
then
	action=start
elif [ $prog = lsnrstop ]
then
	action=stop
else
	action=status
fi

unset TWO_TASK TNS_ADMIN

pfile=${ORACLE_HOME}/dbs/init${ORACLE_SID:?"must be set"}.ora
spfile=${ORACLE_HOME}/dbs/spfile$ORACLE_SID.ora

rcdir=/etc/default
if [ `uname -s` = "HP-UX" -a -z "$iflg" ]
then
	rcdir=/etc/rc.config.d
	[+eval+] `grep -s '^ORA_CRS_HOME=' /etc/cmcluster/*/oc.conf`
	if [ "$ORA_CRS_HOME" ]
	then
		CRS_ORACLE_HOME=$ORA_CRS_HOME
		PATH=$CRS_ORACLE_HOME/bin:$PATH
		export CRS_ORACLE_HOME PATH
	fi
	
	if cmd=`crs_stat -p 2>&- | awk -F= -v node=\`hostname\` -v sid=$ORACLE_SID -v action=$action '
		BEGIN { rc = 1 }
		/^NAME=ora\.[^\.]+\.[^\.]+\.lsnr/,/^[ \t]*$/ { 
			if ($1 == "NAME")
				resource=$2
			if ($1 == "HOSTING_MEMBERS") {
				if ($2 == node) {
					split(resource, a, /\./)
					if (a[3] ~ "_*"sid"$" || a[3] ~ "^"sid"$") {
						printf("srvctl %s listener ", action)
						printf("-n %s -l %s\n", a[2], a[3])
						rc = 0	# found
					}
				}
			}
		}
		END { exit rc }'`
	then
		cmd="$ORA_CRS_HOME/bin/$cmd"
	fi
fi

if [ -f $pfile -o -f $spfile ]
then
	# Is there an appsutil script?

	[ -r $rcdir/oracle ] && . $rcdir/oracle

	[ "$DBCTX" ] || [+eval+] `BINDIR/ctx -f -s $ORACLE_SID 2>/dev/null`

	if [ -f "$DBCTX" ]
	then
		ctrl_script=`BINDIR/ctxvar -if s_dlsnctrl $DBCTX`
	fi

	id=`expr "\`id\`" : '[^(]\{1,\}(\([^ ]\{1,\}\))'`   # non XPG4 id
	if [ $id = root ]
	then
		owner=`SBINDIR/dbid`
	fi

	if [ "$cmd" ]
	then
		# found a CRS managed listener for this host/SID
		[ $nflg ] || echo $cmd
		[+eval+] $run ${owner:+"su $owner -c ${run:+\\}'"}$cmd${owner:+"${run:+\\}'"}
	elif [ -f "$ctrl_script" ]
	then
		[ $nflg ] || echo $ctrl_script $action $ORACLE_SID
		[+eval+] $run ${owner:+"su $owner -c ${run:+\\}'"}$ctrl_script $action $ORACLE_SID${owner:+"${run:+\\}'"}
	elif [ -f $ORACLE_HOME/network/admin/listener.ora ]
	then
		names=`sed -n 's;^SID_LIST_\([^	 =]\{1,\}\).*;\1;p' \
			$ORACLE_HOME/network/admin/listener.ora`
		set -- $names
		[ $# -eq 0 ] && set -- LISTENER
		for name
		do
			[ $name = LISTENER ] && name=
			[+eval+] $run ${owner:+"su $owner -c ${run:+\\}'"} TNS_ADMIN=$ORACLE_HOME/network/admin \
				$ORACLE_HOME/bin/lsnrctl $action $name${owner:+"${run:+\\}'"}
		done
	else
		[ $run ] || echo Listener not configured for Database \"$ORACLE_SID\".
	fi
else
	[ $run ] || echo Can\'t find init file for Database \"$ORACLE_SID\".
fi
changequote(`,)dnl
