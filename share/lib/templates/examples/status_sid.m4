divert(-1)dnl
changequote([+, +])dnl
# vim:syntax=sh:ts=4:sw=4:
divert(0)dnl
#!/bin/sh -
#ident $Header$
# This file is generated automatically. Any changes you make may go away.

PATH=/bin:/usr/sbin:/usr/bin:/usr/contrib/bin:__BIN__:__SBIN__
export PATH

prog=`basename $0`
action=`expr "\`basename $0 .sh\`" : '\([^_]*\)_.*'`

usage() {
	cat <<!
usage: $prog           report status of services
!
}

while getopts ? opt 2>&-
do
	case $opt in
	\?)	errflg=y
	esac
done
[+shift+] `expr $OPTIND - 1`

[ $errflg ] && { usage >&2; exit 2; }

# STATUS
# To show the type of the node:
	#adstatus -n -s __SID__
# To show enabled services for this node:
	#adstatus -e -s __SID__
# To run the script with the status parameter for the enabled service above:
	#adstatus -s __SID__
# To show running processes:
	QUEUE=y export QUEUE
	adps -s __SID__
changequote(`,)dnl
