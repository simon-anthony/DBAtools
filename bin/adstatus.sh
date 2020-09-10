#!/bin/sh -
#ident $Header$
################################################################################
# adstatus: display status of e-Business Suite services
#
################################################################################
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
PATH=/bin:/usr/bin:BINDIR
[ -d /usr/xpg4/bin ] && PATH=/usr/xpg4/bin:$PATH
export PATH

prog=`basename $0`
 
name=`expr "$prog" : 'ad\(.*\)$' 2>&-`
case "$name" in
kill)
      usage="[-q[-v]|-l|-e|-n] [-k <signal>] [-p] [-u] [-s <SID> [-r <host>]] [-t <TWO_TASK>] [<rpt>]"
      type=$name
      extraopt=k:
      ;;
tns|ps)
      usage="[-q[-v]|-l|-e|-n] [-g <mode>] [-p] [-u] [-w <num>] [-s <SID> [-r <host>]] [-t <TWO_TASK>] [<rpt>]"
      type=$name
      ;;
*)    usage="[-q[-v]|-l|-e|-n] [-a|-d] [-g <mode>] [-p] [-u] [-w <num>] [-s <SID> [-r <host>]] [-t <TWO_TASK>] [-T <type>] [<rpt>]" 
      type=status
      extraopt=T:ad
esac
 
while getopts ng:qvs:r:lepubw:xt:$extraopt opt 2>&-
do
	case $opt in
	n)	nflg=y							# print node information
		[ "$qflg" -o "$eflg" -o "$lflg" ] && errflg=y
		;;
	q)	qflg=y							# quiet mode
		[ "$nflg" -o "$eflg" -o "$lflg" ] && errflg=y
		;;
	v)	vflg=y							# verify mode - implies quiet mode
		[ "$nflg" -o "$eflg" -o "$lflg" ] && errflg=y
		qflg=y
		;;
	e)	[ "$qflg" -o "$lflg" -o "$nflg" ] && errflg=y
		eflg=y							# display output from XML file
		;;
	l)	lflg=y							# list reports 
		[ "$qflg" -o "$eflg" -o "$nflg" ] && errflg=y
		;;
	r)	rhost="$OPTARG"
		rflg=y
		;;
	a)	[ $dflg ] && errflg=y			# restrict report to apps node services
		aflg=y
		;;
	d)	[ $aflg ] && errflg=y			# restrict report to db node services
		dflg=y
		;;
	s)	ORACLE_SID=$OPTARG export ORACLE_SID
		sflg=y							# SID
		;;
	t)	TWO_TASK=$OPTARG				# TWO_TASK override
		tflg=y
		;;
	T)	type=$OPTARG					# type of reports
		Tflg=y
		;;
	p)	pflg="-p"						# use ps listing in preference to ptree
		;;
	u|b)
		uflg="-u"						# use ps(1B) even in ptree listing
		;;
	w)	width=`expr "$OPTARG" : '\([0-9]\{1,\}\)$'` || {
			usage="$usage\n\twhere <num> is positive integer"
			if [ "$width" = 0 ]
			then
				width=`tput cols`
				uflg="-u"
			else
				errflg=y
			fi
		}
		wflg=y
		;;
	x)	xflg=y							# scripts need not be executable
		mode=-r
		;;
	g)	gflg=y							# graphics mode for headings
		cap=$OPTARG 
		case $OPTARG in 
		bold)
			cap=bold capoff=sgr0;;
		smso|so|standout)
			cap=smso capoff=rmso;;
		smul|ul|underline|underscore)
			cap=smul capoff=rmul;;
		off)
			cap=off;;
		*)	usage="$usage\n\twhere <mode> is [bold|standout|underline|off]"
			errflg=y
		esac
		;;
	k)	signal=$OPTARG					# signal
		case $signal in
		[1-9]|1[0-5|HUP|INT|QUIT|ABRT|KILL|TERM) : ;;
		*)	errflg=y
		esac
		kflg="-k$signal"
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`
 
[ "$rflg" -a ! "$sflg" ] && errflg=y  # if -r then -s is required
 
[ $errflg ] && { echo usage: $prog $usage >&2; exit 2; }

[ $xflg ] || mode=-x
 
# xmlvar: return value of xml variable in context file
#  - really should use a parser
#   $1 - variable name  $2 - XML file
#
xmlvar()
{
	[ -r "$2" ] || { echo "xmlvar: cannot open $2" >&2; exit 1; }
	sed -n 's;.*[ 	]\{1,\}oa_var="'$1'"[^>]*>\(.*\)<.*;\1;p' $2
}
 
#####
#
# Set up owner of this database
 
[ $sflg ] && unset APPL_TOP COMMON_TOP DB_TOP ORA_TOP INST_TOP
: ${ORACLE_SID:=$TWO_TASK}
[ $tflg ] || : ${TWO_TASK:=$ORACLE_SID}
export ORACLE_SID TWO_TASK
 
[ "$ORACLE_SID" ] || {
      echo $prog: TWO_TASK or ORACLE_SID not set or cannot be determined >&2
      exit 1
}
 
ssh="ssh -o BatchMode=yes -o ChallengeResponseAuthentication=no"
: ${RSH:=$ssh}
if [ $rflg ]
then
      $RSH $rhost : >/dev/null 2>&1 || {
            echo "$prog: User not equivalenced on host '$rhost'." >&2
            echo "Add entry in .rhosts or hosts.equiv(4) on '$rhost'." >&2
            echo "Or add public key access on '$rhost'." >&2
            exit 1
      }
      $RSH $rhost "
      if [ ! -x BINDIR/adstatus ]
      then
            echo "$prog: package is not installed on host '$rhost'." >&2
            exit 1
      fi
      BINDIR/ad$type ${qflg:+-q} ${vflg:+-v} ${lflg:+-l} ${eflg:+-e} ${nflg:+-n} ${aflg:+-a}${dflg:+-d} -s $ORACLE_SID ${pflg:+-p} ${uflg:+-u} ${gflg:+-g$cap} ${Tflg:+-T$type} ${wflg:+-w$width} $*"
      exit 
fi
 
host=`hostname`
[ -r /etc/default/oracle ] && . /etc/default/oracle
[ "$HOSTNAME" ] && host=$HOSTNAME export HOSTNAME
 
# Determine XML configuration file(s)
eval `ctx -f -s $ORACLE_SID 2>&-`
appctxfile=$APPCTX
dbctxfile=$DBCTX

eval `ctx -s $ORACLE_SID 2>&-`
[ "$APPCTX" ] && export APPCTX || unset APPCTX
[ "$DBCTX" ]  && export DBCTX  || unset DBCTX

tmpfile=`mktemp`

if [ $APPCTX ]
then
	s_com=`ctxvar -if s_com $appctxfile`
	s_config_home=`ctxvar -if s_config_home $appctxfile`
	s_weboh_oh=`ctxvar -if s_weboh_oh $appctxfile`
	CP_TWO_TASK=`ctxvar -if s_cp_twotask $appctxfile` export CP_TWO_TASK
	s_apps_version=`ctxvar -if s_apps_version $appctxfile`
	release=`echo $s_apps_version | cut -f1 -d.`

	if [ -d $s_config_home/admin/scripts ]
	then
		cdir=$s_config_home/admin/scripts
		s_com=$s_config_home
	else
		cdir=$s_com/admin/scripts/$APPCTX
	fi
	if [ ! -r $cdir ]
	then
		echo $prog: cannot access \'$cdir\' >&2
		exit 1
	fi
	UTIL_DIR=$cdir
	export UTIL_DIR

	s_tools_oh=`ctxvar -if s_tools_oh $appctxfile`
	s_weboh_oh=`ctxvar -if s_weboh_oh $appctxfile`
	if [ "$s_tools_oh" ]
	then
		ORACLE_HOME=$s_tools_oh
	else
		ORACLE_HOME=$s_weboh_oh
	fi
	export ORACLE_HOME

	ID=`rwid -o $s_tools_oh/.. $ORACLE_SID 2>&-`

	if [ ! "$dflg"  ]
	then
		ORA_NLS10=DATADIR/nlsdata BINDIR/adsvcctl -r$release -e $appctxfile |
		awk '
			/<Service>/,/^ServiceControl is exiting/ {
				if (NF >= 2 && ($(NF-1) ~ /.*.sh$/ || $(NF) ~ /.*.sh$/))
				print
			}' | sed '/Undefined/d' > $tmpfile
	fi
fi
 
if [ "$DBCTX" ]
then
	ddir=`dirname \`ctxvar -if s_dlsnctrl $dbctxfile\``

	if [ "A$ddir" = "A" ]
	then	
		echo $prog: cannot determine s_dlsnctrl from $dbctxfile >&2
		exit 1
	fi
	if [ ! -r $ddir ]
	then
		echo $prog: cannot access \'$ddir\' >&2
		exit 1
	fi

	[ "$APPCTX" ] || APPCTX=$DBCTX
	[ "$appctxfile" ] || appctxfile=$dbctxfile

	s_db_oh=`ctxvar -if s_db_oh $dbctxfile`
	ORACLE_HOME=$s_db_oh export ORACLE_HOME

	s_dbSid=`ctxvar -if s_dbSid $dbctxfile`

	ID=`rwid -d $s_db_oh/.. $ORACLE_SID 2>&-`
	if [ ! "$aflg" ]
	then
		ORA_NLS10=DATADIR/nlsdata BINDIR/adsvcctl -e $dbctxfile |
		awk '
			/<Service>/,/^ServiceControl is exiting/ {
				if (NF >= 2 && ($(NF-1) ~ /.*.sh$/ || $(NF) ~ /.*.sh$/))
				print
			}' >> $tmpfile
	fi
else
	if [ -z "$APPCTX" ]
	then
		echo $prog: cannot determine any XML context file location for \"$ORACLE_SID\"  >&2
		rm -f $tmpfile
		exit 1
	fi
fi
APPCTXFILE=$appctxfile
DBCTXFILE=$dbctxfile
export APPCTX APPCTXFILE DBCTXFILE

if [ ! "$DBCTX" -a ! "$APPCTX" ]
then
      echo $prog: cannot find apps or database context for \"$ORACLE_SID\" >&2
	  rm -f $tmpfile
      exit 1
fi
 
[ $ID ] || {
      echo $prog: cannot determine owner of database \"$ORACLE_SID\" >&2
	  rm -f $tmpfile
      exit 1
}
export ID
 
enabled_services=`awk '
	$(NF) != "Disabled" && $0 ~ /\.sh/ {
		printf "%s\n", substr($(NF),1,match($(NF),".sh")-1)
	}' $tmpfile`
 
if [ $eflg ]
then
	cat $tmpfile
	rm -f $tmpfile
	exit 0
fi

exists() {
	[ -r $ddir/${1}.sh -o -r $cdir/${1}.sh ] && return 0
	[ -r $ddir/ad${1}ctl.sh -o -r $cdir/ad${1}ctl.sh ] && return 0
	return 1
}
 
enabled() {
	if [ $mode $cdir/$1.sh -o $mode $cdir/ad${1}ctl.sh ]
	then
		for _i in $enabled_services
		do
			[ "$1" = $_i -o ad${1}ctl = $_i ] && return 0
		done
		return 1
	fi
	if [ $mode $ddir/$1.sh -o $mode $ddir/ad${1}ctl.sh ]
	then
		return 0
	fi
	return 1
}
 
#####
#
# Listing 

if [ $lflg ]
then
	if [ "$extraopt" ]
	then
		types=`ls DATADIR/lib/rpt | sed '/log/d; /fnd/d'`
	else
		types=$type
	fi

	[ "$extraopt" -a ! "$Tflg" ] && printf "%-5s %-5s\n" Type Name
	for _type in $types
	do
		if [ "$Tflg" -a $_type != $type ]
		then
			continue
		fi
		[ "$extraopt" -a ! "$Tflg" ] && printf "%s\n" $_type
		reports=`ls DATADIR/lib/rpt/$_type/*/prog 2>&- | sed 's;.*/\(.*\)/prog;\1;'`
		if [ $_type = ps ]
		then
			for service in $enabled_services
			do
				com=
				found=
				for report in $reports
				do
					if [ "$service" = $report -o "$service" = ad${report}ctl ]
					then
						com=`cat -s DATADIR/lib/rpt/$_type/$report/comment`
						found=$report
					else
						continue
					fi

					dis=
					if enabled $report
					then
						if tput ${cap:-bold} 2>&-
						then
							off=`tput ${capoff:-sgr0}`
						else 
							off=
						fi
					else
						if [ "$cap" != "off" ] && tput ${capoff:-sgr0} 2>&-
						then
							:
						else
							dis=y
						fi
					fi
					[ $found ] && break
				done
				if [ ! "$found" ]
				then
					# print ad${report}ctl if possible
					service=`expr "$service" : 'ad\(.*\)ctl' \| "$service"`
					printf "%12s$off - %s${dis:+ [Disabled]}\n" $service "* no report *"
				else
					printf "%12s$off - %s${dis:+ [Disabled]}\n" $found "$com"
				fi
			done
		else
			for report in $reports
			do
				exists $report || continue
				com=`cat -s DATADIR/lib/rpt/$_type/$report/comment`
				dis=
				if enabled $report
				then
					if tput ${cap:-bold} 2>&-
					then
						off=`tput ${capoff:-sgr0}`
					else 
						off=
					fi
				else
					if [ "$cap" != "off" ] && tput ${capoff:-sgr0} 2>&-
					then
						:
					else
						dis=y
					fi
				fi
				printf "%12s$off - %s${dis:+ [Disabled]}\n" $report "$com"
			done
		fi
	done
	rm -f $tmpfile
	exit 0
fi

reports=`ls DATADIR/lib/rpt/$type/*/prog 2>&- | sed 's;.*/\(.*\)/prog;\1;'`

if [ $# -gt 0 ]
then
	args=
	for i
	do
		found=
		for j in $reports
		do
			if [ $i = $j ]
			then
				enabled $i && found=y
				break
			fi
		done
		if [ ! "$found" ]
		then
			echo $prog: invalid report \`$i\` for \'$ORACLE_SID\' >&2
			echo Use \`-l\` to list valid reports for this database >&2
			exit 2
		fi
		[ "$_reports" ] && _reports="$_reports $i" || _reports=$i

		if expr "$i" : 'ad.*ctl$' >/dev/null
		then
			args="$args ad${i}ctl"
		else
			args="$args $i"
		fi
	done
	reports="$_reports"
	enabled_services="$args"
fi

#####
#
# Set up screen width

if [ $wflg ]
then
  ll=$width
else
  ll=80
  [ -t ] && ll=`tput cols`
fi

parse() {
  set -- `sed '
		s;[   ]*;;g
		s;-le;<=;g
		s;-ge;>=;g
		s;-lt;<;g
		s;-gt;>;g
		s;-ne;!=;g
		s;<>;!=;g
		s;-eq;==*;g
		s;==*;==;g
		s;\([<>!]\)==*;\1=;g
		s;\([<>!=]\{1,\}\); \1 ;g' $1`

	[ $# -gt 2 ] && return 1

	op="==" num=$1

	if [ $# -gt 1 ]
	then
		if expr A"$1" : A'[><!=]\{1,\}' >&-
		then
			op=$1
		else
			return 2
		fi
		num=$2
	fi
	case "$num" in
	[0-9]*)
			: ;;
	*)		return 3
	esac
	echo $op $num
}

if [ cap != off ]
then
	if tput ${cap:-bold} >/dev/null 2>&1
	then
		CAP=`tput ${cap:-bold}`             # available to reports
		CAPOFF=`tput ${capoff:-sgr0}`
		export CAP CAPOFF
		graphics=y  # have graphics
	fi
fi

if [ $nflg ]
then
	host=
	if [ -n "$dbctxfile" -a "$dbctxfile" != "$appctxfile" ]
	then
		db_host=`ctxvar -if s_hostname $dbctxfile`
		app_host=`ctxvar -if s_hostname $appctxfile`
	else
		host=`ctxvar -if s_hostname $appctxfile`
	fi

	s_isForms=`ctxvar -if s_isForms $appctxfile`
	[ $dbctxfile ] && s_isDB=`ctxvar -if s_isDB $dbctxfile` || s_isDB=`ctxvar -if s_isDB $appctxfile`
	s_isAdmin=`ctxvar -if s_isAdmin $appctxfile`
	s_isWeb=`ctxvar -if s_isWeb $appctxfile`
	s_isConc=`ctxvar -if s_isConc $appctxfile`

	boldyes() {
		case $1 in
		YES|Y)	echo ${CAP}$1$CAPOFF ;;
		*)		echo $1 ;;
		esac
	}

	echo "${CAP}Node$CAPOFF \c"
	echo $host${app_host:+$app_host/}$db_host

	echo "Database             : `boldyes $s_isDB`"
	echo "Concurrent Processing: `boldyes $s_isConc`"
	echo "Administrative Node  : `boldyes $s_isAdmin`"
	echo "Web Server           : `boldyes $s_isWeb`"
	echo "Forms Server         : `boldyes $s_isForms`\n"

	rm -f $tmpfile
	exit 0
fi

if [ "$type" = status ]
then
	for service in $enabled_services
	do
		com=
		found=
		multi=

		com=`awk '
			$(NF) == "Disabled" {
				next
			}
			$(NF) == "'${service}'.sh" {
				printf "%s\n", substr($0, 3, match($0, $(NF))-4)
				exit
			}' $tmpfile`

		if [ ! "$qflg" ]
		then
			[ $multi ] && echo "$pad"
			if [ "$graphics" ]
			then
				  echo $CAP$com$CAPOFF
			else
				  echo $com |
				  awk '{
						print
						for (i = 0; i < length($0); i++)
							  printf("-")
						printf("\n")
				  }'
			fi
		fi
		if [ $service = adcmctl ]
		then
			APPS_UNPW=apps/`getappspass -f -s $ORACLE_SID 2>&-`
			[ -x $cdir/$service.sh ] && $cdir/$service.sh status $APPS_UNPW
		elif [ $service = addlnctl ]
		then
			[ -x $ddir/$service.sh ] && $ddir/$service.sh status $s_dbSid
		else
			[ -x $cdir/$service.sh ] && $cdir/$service.sh status 
			[ -x $ddir/$service.sh ] && $ddir/$service.sh status 
		fi
		errs=`expr ${errs:=0} + $?`
		multi=y
	done
fi

multi=
for report in $reports
do
	enabled $report || continue

	if [ ! "$qflg" ]
	then
		[ $multi ] && echo "$pad"
		if [ -r DATADIR/lib/rpt/$type/$report/comment ]
		then
			com=`cat DATADIR/lib/rpt/$type/$report/comment`
		else
			com=`awk '
				$(NF) == "Disabled" {
					next
				}
				$(NF) == "'${report}'.sh" || $(NF) == "ad'${report}'ctl.sh" { 
					printf "%s\n", substr($0, 3, match($0, $(NF))-4)
					exit
				}' $tmpfile`
		fi
		if [ "$graphics" ]
		then
			  echo $CAP$com$CAPOFF`[ $type = ps ] && [ "$EXPECT" ] && echo " [$expect]"`
		else
			  echo $com |
			  awk '{
					print
					for (i = 0; i < length($0); i++)
						  printf("-")
					printf("\n")
			  }'
		fi
	fi
	sh -p DATADIR/lib/rpt/$type/$report/prog $pflg $uflg $kflg |
	awk '
		$0 !~ /^[ \t]*[PU]ID|^Configured|^Processes/ {
			  pcount++
		}
		{
			  if (!length("'$qflg'"))
					print substr($0, 1, '$ll'-1)
		}'
	multi=y
done
rm -f $tmpfile
exit $errs
