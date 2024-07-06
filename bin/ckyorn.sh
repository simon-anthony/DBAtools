#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:

PATH=/usr/bin export PATH

Qflg= Wflg= dflg= hflg= eflg= hflg= kflg= sflg= errflg=

default=
prompt="Yes or No"
values="[y,n,?,q]"
error="ERROR - Please enter yes or no."
help="To respond in the affirmative, enter y, yes, Y or YES.\nTo respond in the negative, enter n, no, N or NO."
pid=
signal=

trap "exit 1" 1 2 3 15

while getopts QW:d:h:e:p:k:s: opt 2>&-
do
	case $opt in
	Q)	values="[y,n,?]"		# do not allow 'q'
		Qflg=y
		;;
	W)	Wflg=y
		;;
	d)	default="$OPTARG"		# default is not validated
		dflg=y
		;;
	h)	help="$OPTARG" 
		hflg=y
		;;
	e)	error="$OPTARG" 
		eflg=y
		;;
	p)	prompt="$OPTARG" 
		pflg=y
		;;
	k)	pid="$OPTARG"			# pid to send signal if 'q' 
		kflg=y
		;;
	s)	signal="$OPTARG" 		# signal for -k
		sflg=y
		;;
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ "$sflg" -a ! "$kflg" ] && errflg=y

if [ $errflg ]
then
	printf "usage: ckyorn [-Q] [-W width] [-d default] [-h help] [-e error] [-p prompt] [-k pid[-s signal]]\n" >&2
	exit 2
fi

while true
do
	echo "$prompt $values${default:+ default '$default'}: \c" >&2
	read ans

	[ -z "$ans" -a "$dflg" ] && ans="$default"

	case "$ans" in
	y|Y|[Yy][Ee][Ss])
		echo $ans; return 0
		;;
	n|N|[Nn][Oo])
		echo $ans; return 0
		;;
	\?)	
		printf "$help\n" >&2
		;;
	[Qq])	
		[ $Qflg ] && { printf "$error\n" >&2; continue; }
		[ $kflg ] && kill -${signal:-15} $pid
		return 3
		;;
	*)	printf "$error\n" >&2
	esac
done
