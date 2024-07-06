changequote([+, +])dnl		# shell quotation marks need protecting
#!/usr/bin/ksh 
# vim:syntax=sh:ts=4:sw=4:
################################################################################
#
# Description:
#  adstart script - adjunct to dbstart
#  adstop script  - adjunct to dbshut
#  Locate the eBusiness autoconfig start/stop scripts and execute them.
#  Expect ORACLE_SID in the environment.
#  Only acts on one database, for which the invoking user must be the owner.
#
################################################################################

: ${ORACLE_SID:?"must be set"}	# masquerading as s_systemname

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
usage="[-n][-c|-s]"

while getopts ncs opt 2>&-
do
	case $opt in
	n)	nflg=y		
		run=echo
		;;
	c)	cflg=y			# Use concurrent manager operator defined in context
		[ $sflg ] && errflg=y
		;;
	s)	sflg=y			# user (customised) "-secureapps" option 
		[ $cflg ] && errflg=y
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

if [ $prog = adstart ]
then
	script=adstrtal
else
	script=adstpall
fi

unset TWO_TASK APPL_TOP 

# Apps 11i is server partitioned - so ORACLE_SID is not guaranteed to be set

# ORACLE_SID is set by adstart, if it differs from TWO_TASK, TWO_TASK is
# the database name, but ORACLE_SID is the configuration context

# Determine XML configuration file(s)

[+eval+] `BINDIR/ctx -f -s $ORACLE_SID`
appctxfile=$APPCTX
dbctxfile=$DBCTX

if [ "X$appctxfile" = "X" ]
then
	echo $prog: cannot read context file $appctxfile >&2
	exit 1
fi

[+eval+] `BINDIR/ctx -s $ORACLE_SID`
[ "$APPCTX" ] && export APPCTX || unset APPCTX
[ "$DBCTX" ]  && export DBCTX  || unset DBCTX

s_com=`BINDIR/ctxvar -if s_com $appctxfile`
s_config_home=`BINDIR/ctxvar -if s_config_home $appctxfile`
s_apps_version=`BINDIR/ctxvar -if s_apps_version $appctxfile`
release=`echo $s_apps_version | cut -f1 -d.`

if [ -d $s_config_home/admin/scripts ]
then
	utildir=$s_config_home/admin/scripts
elif [ -d $s_com/admin/scripts/$APPCTX ]
then
	utildir=$s_com/admin/scripts/$APPCTX
elif [ "$AUTOCONFIG" != NO ]
then
	echo No directory $s_com/admin/scripts/$APPCTX
	exit 1
fi

if [ ! -f $utildir/$script.sh ]
then
	echo Cannot locate script \'$script\' for \"$ORACLE_SID\"
	exit 1
fi

filter() {
	if [ $nflg ]
	then
		cat -
	else
		outfile=`mktemp`
		sed -n '/Executing service control script:/,/admin\/scripts/ {
				/admin\/scripts/ p
			}
			/exiting with status/ p
			/All enabled services .* this node are/,$ {
				/^[ 	]*$/ { d; n; }
				/Check logfile/ {
					s; for details;;
					w '$outfile'
				}
				p
			}' 
		read line < $outfile; set -- $line; logfile=$3
		awk '
		/<Service>/,/^ServiceControl is exiting/ {
			if (NF >= 2 && ($(NF-1) ~ /.*.sh$/ || $(NF) ~ /.*.sh$/) &&
				$(NF) != "Disabled")
				print
		}' $logfile
		rm -f $outfile
	fi
}

id=`expr "\`id\`" : '[^(]\{1,\}(\([^ ]\{1,\}\))'`   # non XPG4 id
owner=`BINDIR/ctxvar -if s_appsuser $appctxfile`
if [ $id != root ]
then
    if [ "$owner" != $id ]
    then
        echo You are not the owner of $ORACLE_SID >&2
        exit 1
    fi
    unset owner
fi

if [ "$run" -a -f $utildir/gsmstart.sh ]
then
	echo The following Services are enabled:
	ORA_NLS10=DATADIR/nlsdata BINDIR/adsvcctl -r$release -e $appctxfile |
	awk '
	/<Service>/,/^ServiceControl is exiting/ {
		if (NF >= 2 && ($(NF-1) ~ /.*.sh$/ || $(NF) ~ /.*.sh$/) &&
			$(NF) != "Disabled")
			print
	}' 
fi

s_dbSid=`BINDIR/ctxvar -if s_dbSid $appctxfile`
s_weboh_oh=`BINDIR/ctxvar -if s_weboh_oh $appctxfile`
s_apps_version=`BINDIR/ctxvar -if s_apps_version $appctxfile`

if expr "$s_apps_version" : '12\.2\.' >&-
then
	[ $cflg ] || sflg=y	# requires WebLogic password
	if [ ! "$sflg" ]
	then
		wlspassword=`adpasswd -w -s $ORACLE_SID`
	fi
fi


if [ $cflg ]
then
	if cp_unpw=`adcpusr -s $ORACLE_SID`
	then
		APPS_UNPW=$cp_unpw
	fi
fi

if [ $sflg ]
then
	[+eval+] $run ${owner:+"su $owner -c ${run:+\\}'"}\
	$utildir/$script.sh -secure${owner:+"${run:+\\}'"} | filter
elif [ $cflg ]
then
	[+eval+] $run ${owner:+"su $owner -c ${run:+\\}'"}\
	{ echo ${APPS_UNPW%/*} \; echo ${APPS_UNPW#*/} \; echo $wlspassword\; } | $utildir/$script.sh -secureapps${owner:+"${run:+\\}'"} | filter
else
	[+eval+] $run ${owner:+"su $owner -c ${run:+\\}'"}\
	$utildir/$script.sh ${APPS_UNPW:=apps/apps}${owner:+"${run:+\\}'"} | filter
fi
changequote(`,)dnl
