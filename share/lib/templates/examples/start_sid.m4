divert(-1)dnl
changequote([+, +])dnl
# vim:syntax=sh:ts=4:sw=4:
divert(0)dnl
#!/bin/sh -
#ident $Header$
# This file is generated automatically. Any changes you make may go away.

PATH=/usr/sbin:/usr/bin:/usr/contrib/bin:__BIN__:__SBIN__
export PATH

prog=`basename $0`
action=`expr "\`basename $0 .sh\`" : '\([^_]*\)_.*'`

usage() {
	cat <<!
usage: $prog [-r]    $action database and apps tiers
       $prog -d      $action only database tier
       $prog -a [-r] $action only apps tier

   -r   Include remote applications tiers in $action.
!
}

while getopts cdar opt 2>&-
do
	case $opt in
	d)	dflg=y		# start only database
		[ $aflg ] && errflg=y
		;;
	a)	aflg=y		# start only apps tiers
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

if [ "$action" = start -a ! "$aflg" ]
then
__STARTSTOP_LISTENERS__
	__SBIN__/dbstart __SID__
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
	__SBIN__/dbshut __SID__
__STARTSTOP_LISTENERS__
fi

exit 0
changequote(`,)dnl
