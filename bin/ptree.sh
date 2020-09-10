#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
PATH=/bin export PATH

while getopts xs opt 2>&-
do
	case $opt in
	x)	xflg=y ;;
	s)	sflg=y ;;
	\?)	:
	esac
done
shift `expr $OPTIND - 1`

if [ $xflg ]
then
	ptree ${sflg:+-s} $* | awk '
	{
		p = match($0, /[^0-9 ]/)
		cmd = sprintf("UNIX95=y ps -x -o comm= -p %d", $1)
		printf("%s", substr($0, 1, p-1))
		comm = system(cmd)
	}'
	exit 0
else
	exec ptree ${sflg:+-s} $*
fi
