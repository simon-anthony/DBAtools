#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
# Copyright (C) 2008 Simon Anthony
# 
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License or, (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program. If not see <http://www.gnu.org/licenses/>>
# 
PATH=/usr/sbin:/usr/bin:BINDIR:SBINDIR
export PATH
unset SHLIB_PATH LD_LIBRARY_PATH ORACLE_HOME

prog=`basename $0`
usage() {
cat <<!
usage: $prog [-r apps_node_name]... [-p n] [-l] -s SID
!
[ $help ] && cat <<!

   -r apps_node_name  A list of "remote" applications tier nodes that is 
                      in addition to any applications tier nodes in the
                      cluster to be added to the start/stop scripts.

                      Nodes can be specified -r node1 -r node2 or -r node1,node2

   -p n               Start/stop applications nodes in parallel with a pause
                      of n seconds.

   -s SID             The name of the database.

   -l                 Install scripts on local node only.

!
}

while getopts s:r:p:l opt 2>&-
do
	case $opt in
	s)	sid=$OPTARG
		sflg=y	# SID
		unset TWO_TASK
		;;
	r)	appsnodes="$appsnodes `echo \"$OPTARG\" | sed 's;,; ;g'`"
		rflg=y
		;;
	p)	if sleeptime=`expr "$OPTARG" : '\([0-9]\{1,\}\)$'`
		then
			pflg=y
		else
			errflg=y
		fi
		;;
	l)	lflg=y	# run on local node only
		;;
	\?)	errflg=y
		help=y
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 0 ] || errflg=y
[ $sflg ]    || errflg=y

[ $errflg ] && { usage >&2; exit 2; }

tempdir=`mktemp`
mkdir $tempdir
pkg=PRODUCT

BINDIR/adstatus -g off -n -s $sid > $tempdir/$prog$$ 2>&-
isDB=`awk '/Database/ { print $(NF) }' $tempdir/$prog$$`
isApps=`awk '/Concurrent Processing/ { print $(NF) }' $tempdir/$prog$$`

cd ${HOME:?not set} || { echo $prog: cannot chdir to $HOME; exit 1; }
[ -w . ] || { echo $prog: cannot write to $HOME; exit 1; }
rm -f start_$sid.sh stop_$sid.sh status_$sid.sh start_mm_$sid.sh stop_mm_$sid.sh

if [ "$isApps" != YES ]
then
    isApps=`awk '/Web Server/ { print $(NF) }' $tempdir/$prog$$`
fi

ssh="ssh -o BatchMode=yes -o ChallengeResponseAuthentication=no"
scp="scp -o BatchMode=yes -o ChallengeResponseAuthentication=no"

# xmlvar: return value of xml variable in context file
#  - really should use a parser
#   $1 - variable name  $2 - XML file
#
xmlvar()
{
	[ $# -eq 2 ] || {
		echo "usage: xmlvar <variable name> <XML context file>" >&2;
		return 1; }
	[ -r "$2" ] || { echo "xmlvar: cannot open $2" >&2; return 1; }

	_ctx=`ident $2 2> /dev/null | awk '
		/Header:/ { print substr($2, 1, match($2, /\./)-1); exit }'`

	case $_ctx in
	adxdbctx) : ;; # Apps context file
	adxmlctx) : ;; # DB context file
	*)	echo "xmlvar: invalid XML context file '$2'"; return 1;
	esac

	sed -n 's;.*[ 	]\{1,\}oa_var="'$1'"[^>]*>\(.*\)<.*;\1;p' $2
}

if [ "$sid" = crs -o "$sid" = CRS ]
then
	sid="CRS"
	mnp="crsp-slvm"
	tmpl="crs"
	isDB=YES
else
	mnp=`BINDIR/racp -s $sid`		# multi-node package name
	tmpl="sid"
fi

list() {
	OPTIND=1
	_tflg= _tab=
	while getopts t opt $* 2>&-
	do
		case $opt in
		t)		_tflg=y; _tab="$_tab\t" ;;
		esac
	done
	shift `expr $OPTIND - 1`

	i=0
	while [[ $i -lt $nnodes ]]
	do
		case $1 in
		lsnrs)
		print "${_tab}srvctl \$action listener -n ${nodes[$i]} -l ${lsnrs[$i]}"
		;;
		services)
		if adstatus -n -r ${nodes[$i]} -s $sid | awk '
			$1 != "Database" && $(NF) == "YES" { n = 1; exit; }
			END { if (n) exit 0; else exit 1; }'
		then
			print "${_tab}ssh ${nodes[$i]} \"SBINDIR/ad\$action $sid\"${pflg:+ & sleep $sleeptime}"
		fi
		;;
		nodeargs)
		[ $i -eq 0 ] \
			&& print -n -- "-n ${nodes[$i]}" \
			|| print -n -- " -n ${nodes[$i]}"
		;;
		status_instance)
		print "${_tab}srvctl status instance -d $sid -i ${lsnrs[$i]}"
		;;
		status_nodeapps)
		print "${_tab}srvctl status nodeapps -n ${nodes[$i]}"
		;;
		esac
		(( i += 1 ))
	done
	if [ $1 = services ]
	then
		[ "$pflg" -a $i -gt 0 ] && print "${_tab}wait"
	fi
	if [ $1 = remoteservices ]
	then
		i=0
		while [[ $i -lt $nnodes2 ]]
		do
			if adstatus -n -r ${nodes2[$i]} -s $sid | awk '
				$1 != "Database" && $(NF) == "YES" { n = 1; exit; }
				END { if (n) exit 0; else exit 1; }'
			then
				print "${_tab}ssh ${nodes2[$i]} \"SBINDIR/ad\$action $sid\"${pflg:+ & sleep $sleeptime}"
			fi
			(( i += 1 ))
		done
		[ "$pflg" -a $i -gt 0 ] && print "${_tab}wait"
	fi
}

if [ "$isDB" = YES ]         # Cases 1 and 2
then
	# find instance name on current node
	if [ "$mnp" != "crsp-slvm" ]
	then
		dbctxfile=`BINDIR/getctx -d -s $sid` 2>&-
		[ $dbctxfile ] || {
			echo "ERROR: cannot determine context file for $sid on $node" >&2
			exit 1
		}
		s_instName=`xmlvar s_instName $dbctxfile`
	fi
	
	if [ "$mnp" ]
	then
		eval `grep '^ORA_CRS_HOME=' /etc/cmcluster/*/oc.conf`	# set the last found
		if [ "$ORA_CRS_HOME" ] 
		then
			PATH=$ORA_CRS_HOME/bin:$PATH
			export ORA_CRS_HOME PATH
		else
			echo ERROR: cannot locate ORA_CRS_HOME >&2; exit 1
		fi

		if clnodes=`cmviewcl -v -f line -p $mnp | awk -F\| '/^node:/ { print $2 }' |
			while read line
			do
				eval $line
				if [ -n "$name" -a "$name" != "$oldname" ]
				then
					print $name; oldname=$name
				fi
			done`
		then
			:
		else
			echo $prog: unable to determine nodes in cluster package $mnp >&2
			exit 1
		fi
		for node in $clnodes $appsnodes
		do
			[ "$DEBUG" ] && echo "$prog: checking package $pkg in installed on $node... \c"
			if /usr/sbin/swlist -l product $pkg @$node >/dev/null 2>&1
			then
				[ "$DEBUG" ] && echo yes
			else
				[ "$DEBUG" ] && echo no
				echo "$prog: package $pkg is not installed on $node" >&2
				exit 1
			fi
		done

		nnodes=0
		for node in $clnodes
		do
			if [ "$mnp" != "crsp-slvm" ]
			then

				dbctxfile=`$ssh $node "BINDIR/getctx -d -s $sid"` 2>&-
				[ $dbctxfile ] || {
					echo "ERROR: cannot determine context file for $sid on $node" >&2
					exit 1
				}
				if $scp -q $node:$dbctxfile $tempdir
				then
					:
				else
					echo "ERROR: failed to copy context file" >&2; exit 1
				fi
				dbctxfile=`basename $dbctxfile`
				s_dbSid=`xmlvar s_dbSid $tempdir/$dbctxfile`
				s_db_oh=`xmlvar s_db_oh $tempdir/$dbctxfile`
				$ssh $node "chmod a-x $s_db_oh/bin/dbstart $s_db_oh/bin/dbshut" 2>&-

				lsnrs[$nnodes]=$s_dbSid
			fi

			nodes[$nnodes]=$node

			(( nnodes += 1 ))
		done
		nnodes2=0
		for node in $appsnodes
		do
			if $ssh $node "exit" >/dev/null 2>&1
			then
				:
			else
				echo "ERROR: cannot ssh to $node" >&2; exit 1
			fi
			nodes2[$nnodes2]=$node

			(( nnodes2 += 1 ))
		done

		list_lsnrs="`list -t lsnrs`"
		list_status_instance="`list -t status_instance`"
		list_status_nodeapps="`list -t status_nodeapps`"
		list_nodeargs="`list nodeargs`"
	else # not mnp
		echo "$prog: $sid is not under the control of SGeRAC"
		nnodes2=0
		for node in $appsnodes
		do
			if $ssh $node "exit" >/dev/null 2>&1
			then
				:
			else
				echo "ERROR: cannot ssh to $node" >&2; exit 1
			fi
			nodes2[$nnodes2]=$node

			(( nnodes2 += 1 ))
		done

		list_lsnrs="	lsnr\$action $sid"
	fi	# SGeRAC

	if [ "$isApps" = YES ]
	then
		echo "Creating scripts for ${mnp:+SGeRAC Cluster }Database and Application Tiers on `hostname`"
		if [ $mnp ]
		then
			adservices="`list -t services`"
		else
			adservices="	ad\$action $sid"
		fi
		[ $rflg ] && adremoteservices="`list -tt remoteservices`"
	else
		echo "Creating scripts for ${mnp:+SGeRAC Cluster }Database Tier on `hostname`"
		[ $rflg ] && adremoteservices="`list -tt remoteservices`"
	fi

	echo status_$sid.sh
	m4 -D__MNP__=$mnp \
	   -D__SID__=$sid \
	   -D__INSTANCE__=$s_instName \
	   -D__BIN__=BINDIR \
	   -D__SBIN__=SBINDIR \
	   -D__ORA_CRS_HOME__=$ORA_CRS_HOME \
	   -D__STARTSTOP_LISTENERS__="$list_lsnrs" \
	   -D__STARTSTOP_SERVICES__="${adservices:-	:}" \
	   -D__LIST_STATUS_INSTANCE__="$list_status_instance" \
	   -D__LIST_STATUS_NODEAPPS__="$list_status_nodeapps" \
	   -D__HOSTNAME__=`hostname` \
	   -D__CMD_CMMODPKG__="	cmmodpkg -v -e $list_nodeargs $mnp" \
	   DATADIR/lib/templates/${mnp:+cm}status_$tmpl.m4 > status_$sid.sh

	echo start_$sid.sh
	m4 -D__MNP__=$mnp \
	   -D__SID__=$sid \
	   -D__INSTANCE__=$s_instName \
	   -D__BIN__=BINDIR \
	   -D__SBIN__=SBINDIR \
	   -D__ORA_CRS_HOME__=$ORA_CRS_HOME \
	   -D__STARTSTOP_LISTENERS__="$list_lsnrs" \
	   -D__STARTSTOP_SERVICES__="${adservices:-	:}" \
	   -D__STARTSTOP_REMOTE_SERVICES__="${adremoteservices:-		:}" \
	   -D__LIST_STATUS_INSTANCE__="$list_status_instance" \
	   -D__LIST_STATUS_NODEAPPS__="$list_status_nodeapps" \
	   -D__HOSTNAME__=`hostname` \
	   -D__CMD_CMMODPKG__="	cmmodpkg -v -e $list_nodeargs $mnp" \
	   DATADIR/lib/templates/${mnp:+cm}start_$tmpl.m4 > start_$sid.sh

elif [ "$isApps" = YES -a "$isDB" = NO ]		# Case 3
then
	echo "Creating scripts for Application Tier on `hostname`"
	adservices="ad\$action $sid"

	echo start_$sid.sh
	m4 -D__SID__=$sid \
	   -D__BIN__=BINDIR \
	   -D__SBIN__=SBINDIR \
	   -D__STARTSTOP_SERVICES__="${adservices:-	:}" \
	   -D__HOSTNAME__=`hostname` \
	   DATADIR/lib/templates/adstart_sid.m4 > start_$sid.sh
	echo status_$sid.sh
	m4 -D__SID__=$sid \
	   -D__BIN__=BINDIR \
	   -D__SBIN__=SBINDIR \
	   -D__HOSTNAME__=`hostname` \
	   DATADIR/lib/templates/adstatus_sid.m4 > status_$sid.sh
fi

ln start_$sid.sh stop_$sid.sh
chmod 700 start_$sid.sh status_$sid.sh
echo stop_$sid.sh

if [ ! "$lflg" ]
then
	[ $rflg ] && ropts="-r `echo ${appsnodes[*]} | sed 's; ;,;g'`"
	for node in $clnodes
	do
		[ $node = `hostname` ] && continue
		$ssh $node "BINDIR/mkscripts -l ${rflg:+$ropts} -s $sid"
	done
	if [ $rflg ]
	then
		for node in $appsnodes
		do
			$ssh $node "BINDIR/mkscripts -l -s $sid"
		done
	fi
fi
