#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:

Qflg= Wflg= dflg= hflg= eflg= hflg= kflg= sflg= errflg=

default=
prompt="Enter an integer"
values="[?,q]"
error="ERROR - Please enter an integer."
help="Please enter an integer."
pid=
signal=
base=10

trap "exit 1" 1 2 3 15

while getopts QW:d:h:e:p:k:s:b: opt 2>&-
do
	case $opt in
	Q)	values="[y,n,?]"		# do not allow 'q'
		Qflg=y
		;;
	W)	Wflg=y
		;;
	b)	base="$OPTARG"			# base
		if expr "$base" : '[0-9]\{1,2\}$' >&- 
		then
			if [ $base -gt 32 -o $base -lt 2 ]
			then
				echo "base must be in rage 2 to 32"
				exit 1
			fi
		else
			echo "invalid format for base"
			exit 1
		fi
		bflg=y
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
	printf "usage: ckint [-Q] [-W width] [-b base] [-d default] [-h help] [-e error] [-p prompt] [-k pid[-s signal]]\n" >&2
	exit 2
fi

cknum() {
	arg=`echo $1 | tr '[:lower:]' '[:upper:]'`
	res=`bc <<-!
		ibase=$base
		$arg
	!`
	case "$res" in
	"syntax error"*)
		return 1
		;;
	*)	echo $res
		return 0
	esac
}

while true
do
	echo "$prompt $values${default:+ default '$default'}: \c" >&2
	read ans

	[ -z "$ans" -a "$dflg" ] && ans="$default"

	case "$ans" in
	[0-9A-Fa-f]*)
		cknum "$ans"  && return 0
		printf "$error\n" >&2
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
