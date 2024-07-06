#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
################################################################################
#
#  Invoked as either dbstart or dbshut to start/stop databases in oratab
#
################################################################################

PATH=/bin:/usr/bin:BINDIR export PATH
: ${TMPDIR:=/tmp}
unset TWO_TASK	
export TMPDIR

[ `uname -s` = "HP-UX" ] && iopt="[-i]"

prog=`basename $0`
usage="[-n] [-e] [-v] [SID...]"

[ $prog = adstart -o $prog = adstop ] && {
	usage="[-n] [-v] [-c] [systemname...]"
	extraopt=c
}

[ $prog = dbshut ] && {
	mode=immediate
	usage="[-n] $iopt [-m immediate|abort|normal|transactional [local]] [-e] [-v] [-g mode] [SID...]"
	[ `uname -s` = "HP-UX" ] && extraopt=m:i || extraopt=m:
}

[ $prog = dbstart ] && {
	usage="[-n] $iopt [-o force] [-o restrict] [-o pfile=<file>] [-o quiet]
			[-m mount|open [read {only|write}]|nomount] [-e] [-v] [-g mode] [SID...]"
	[ `uname -s` = "HP-UX" ] && extraopt=o:m:i || extraopt=o:m:
}
[ $prog = lsnrstart -o $prog = lsnrstop ] && {
	usage="[-n] $iopt [-e] [-v] [SID...]"
	[ `uname -s` = "HP-UX" ] && extraopt=i
}
	

while getopts g:n${extraopt}ev opt 2>/dev/null
do
	case $opt in
	n)	nflg=y			# Echo commands - do not run them
		run=echo
		;;
	c)	cflg=y			# Use concurrent manager operator
		;;
	o)	oflg=y			# Startup options
		case "$OPTARG" in
		force|restrict|quiet)
			eval "$OPTARG"="$OPTARG"
			;;
		pfile=*) 
			pfile=`expr "$OPTARG" : '.*=\(.*\)'`
			;;
		*)
			errflg=y
		esac
		;;
	m)	[ $mflg ] && errflg=y
		mflg=y			# Shutdown/Startup mode
		case "$OPTARG" in
		immediate|abort|normal)
			mode=$OPTARG
			;;
		transactional)
			mode=$OPTARG
			eval local=\$`expr $OPTIND`
			case "$local" in
			local)	OPTIND=`expr $OPTIND + 1` ;;
			*)		local=	# SID or option
			esac
			;;
		nomount)
			[ "$open" -o "$mount" ] && errflg=y
			mode=$OPTARG
			;;
		mount)
			[ "$open" -o "$nomount" ] && errflg=y
			mode=$OPTARG
			;;
		open)
			[ "$mount" -o "$nomount" ] && errflg=y
			mode=$OPTARG
			eval read=\$`expr $OPTIND`
			case "$read" in 
			read)	OPTIND=`expr $OPTIND + 1`
					eval readmode=\$`expr $OPTIND`
					case "$readmode" in
					only)	only=only
							OPTIND=`expr $OPTIND + 1` ;;
					write)	write=write
							OPTIND=`expr $OPTIND + 1` ;;
					*)		errflg=y
					esac ;;
			*)		open=		# must be a SID or an option
			esac
			;;
		#read|write|only|local)
			#eval "$OPTARG"="$OPTARG"
			#;;
		*)
			errflg=y
		esac
		;;
	v)	vflg=y			# Print the database name
		;;
	e)	eflg=y			# Use oraenv to set environment
		;;
	i)	iflg=y			# Ignore SGeRAC Toolkit on HP-UX
		;;
	g)	gflg=y			# Graphics mode for headings
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
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ $prog = dbstart ] && {
	[ \( -n "$write" -o -n "$only" \) -a ! "$read" ] && errflg=y

	[ "$only" -a "$write" ] && errflg=y

	[ "$read" -a  "$mode" != open ] && errflg=y

	mode="$mode $read $write $only"
	opts="$force $restrict ${pfile:+pfile=$pfile} $quiet"
}
[ $prog = dbshut ] && {
	[ "$local" -a "$mode" != transactional ] && errflg=y

	mode="$mode $local"
}

[ $prog = adstart -o $prog = adstop ] && {
	if [ ! "$cflg" ]
	then
		if username=`nbread`
		then
			if password=`nbread`
			then
				APPS_UNPW=$username/$password
				export APPS_UNPW
			else
				# username supplied but no password
				unset APPS_UNPW
			fi
		fi
	fi
}

if [ $errflg ]
then
	echo usage: $prog $usage >&2
	exit 2
fi

# config
#
oratab=`{ ls /var/opt/oracle/oratab || ls /etc/oratab; } 2>/dev/null`

if [ -z "$oratab" -a $# -eq 0  ]
then
	echo $prog: cannot find oratab file >&2
	exit 1
fi

# xmlvar: return value of xml variable in context file
#   $1 - variable name  $2 - XML file
#
xmlvar()
{
	[ -r "$2" ] || { echo "xmlvar: cannot open $2" >&2; return 1; }
	sed -n 's;.*[	 ]\{1,\}oa_var="'$1'"[^>]*>\(.*\)<.*;\1;p' $2
}

tmpfile=`mktemp -p $prog`

if [ $# -gt 0 ]	# construct a subset oratab of the real oratab 
then
	> $tmpfile || { echo $prog: cannot create $tmpfile >&2; exit 1; }
	for sid
	do
		if [ $prog = adstart -o $prog = adstop ]	# could be Apps node
		then
			if eval `BINDIR/ctx -a -f -s $sid 2>/dev/null`
			then
				[ $APPCTX ] && {
					s_tools_oh=`xmlvar s_tools_oh $APPCTX`
					s_weboh_oh=`xmlvar s_weboh_oh $APPCTX`
					if [ "$s_tools_oh" ]
					then
						echo $sid:$s_tools_oh:Y >> $tmpfile
					else
						echo $sid:$s_weboh_oh:Y >> $tmpfile
					fi
				}
				export APPCTX DBCTX
			else
				echo $prog: sid \'$sid\' not in oratab or found as apps node >&2
			fi
			continue
		else
			if eval `BINDIR/ctx -d -f -s $sid 2>/dev/null` # may be Apps RAC DB
			then
				[ $DBCTX ] && {
					sid=`xmlvar s_dbGlnam $DBCTX`
					s_db_oh=`xmlvar s_db_oh $DBCTX`
					echo $sid:$s_db_oh:Y >> $tmpfile
				}
				export APPCTX DBCTX
			elif grep -qs "^$sid:" ${oratab:-/nosuchfile}
			then
				# ensure that DB is started irrespective of value of 3rd field
				# only print the first
				sed -n "/^$sid:/ {
					s;^\($sid\):\(.*\):.*;\1:\2:Y;
					p
					q
					}" $oratab >> $tmpfile
			fi
		fi
	done
	oratab=$tmpfile	# switch oratab
fi

trap 'exit' 1 2 3
case $ORACLE_TRACE in
    T) set -x ;;
esac

if [ cap != off ]
then
	if tput ${cap:-bold} >/dev/null 2>&1
	then
		CAP=`tput ${cap:-bold}`			# available to reports
		CAPOFF=`tput ${capoff:-sgr0}`
		export CAP CAPOFF
		graphics=y	# have graphics
	fi
fi

#
# Loop for every entry in oratab file and and try to start
#
n=0
retval=0

while read line
do
	expr "X$line" : 'X[ 	]*#' \| "X$line" : 'X[ 	]*$' >/dev/null && continue
	set -- `echo "$line" |
		sed -n 's;^\([^:]\{1,\}\):\([^:]\{1,\}\):\([^:]\{1\}\).*;\1 \2 \3;p'`
	if [ ${3:-N} = Y ]
	then
		unset ORACLE_GLNAM
		ORACLE_SID=$1
		[ "$ORACLE_SID" = '*' ] && continue
		if [ $prog = dbstart -o $prog = dbshut -o $prog = lsnrstart -o $prog = lsnrstop -o $prog = dbshm ]
		then
			if eval `BINDIR/ctx -d -f -s $ORACLE_SID 2>/dev/null`
			then
				ORACLE_GLNAM=$ORACLE_SID export ORACLE_GLNAM
				ORACLE_SID=`xmlvar s_dbSid $DBCTX`
			fi
		fi
		export ORACLE_SID
		ORACLE_HOME=$2 export ORACLE_HOME
		[ -d $ORACLE_HOME ] || {
			 echo "$prog: cannot find ORACLE_HOME for \"$ORACLE_SID\" ('$ORACLE_HOME')" >&2
			continue
		}
		unset TWO_TASK
		if [ $eflg ]
		then
			ORAENV_ASK=NO . BINDIR/oraenv
		else
			PATH=$ORACLE_HOME:/bin:/usr/bin:BINDIR export PATH
		fi
		[ $vflg ] && echo $ORACLE_SID:

		[ $n -eq 0 ] && {
			sed -n 's;^[ 	]*header="\(.*\)";\1;p' DATADIR/lib/$prog.sh
			[ $nflg ] && \
				sed -n 's;^[ 	]*nheader="\(.*\)";\1;p' DATADIR/lib/$prog.sh
		}
		if [ $prog = adstart -o $prog = adstop ] 
		then
			sh DATADIR/lib/$prog.sh ${nflg:+-n} ${iflg:+-i} ${cflg:+-c} 
		else
			sh DATADIR/lib/$prog.sh ${nflg:+-n} ${iflg:+-i} $opts $mode 
		fi
		retval=`expr $retval + $?`
		n=`expr $n + 1`
	fi
done < $oratab

rm -f $tmpfile
exit $retval 
