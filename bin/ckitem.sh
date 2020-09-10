#!/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:

PATH=/usr/bin export PATH

Qflg= Wflg= uflg= nflg= oflg= fflg= lflg= iflg= mflg= dflg= hflg= eflg= hflg= kflg= sflg= errflg=

default=
prompt="Enter selection"
values="[?,??,q]"
error="ERROR: Bad numeric choice specification"
serror="ERROR: Entry does not match available menu selection"
help="Enter the number of the menu item you wish to select, the token\nwhich is associated with the menu item, or a partial string which\nuniquely identifies the token for the menu item. Enter ? to\nreprint the menu."
pid=
signal=

trap "exit 1" 1 2 3 15

while getopts QW:unof:l:i:m:d:h:e:p:k:s: opt 2>&-
do
	case $opt in
	Q)	values="[?,??]"		# do not allow 'q'
		Qflg=y
		;;
	W)	Wflg=y
		;;
	u)	uflg=y
		;;
	n)	nflg=y
		;;
	o)	oflg=y
		;;
	f)	[ $fflg ] && errflg=y
		fflg=y
		file="$OPTARG"
		;;
	l)	lflg=y
		label="$OPTARG"
		;;
	i)	iflg=y
		invis="$invis $OPTARG"
		;;
	m)	mflg=y
		case "$OPTARG" in
		[0-9]|[0-9][0-9])
			max=$OPTARG ;;
		*)	errflg=y
		esac
		;;
	d)	default="$OPTARG"		# default is not validated
		dflg=y
		;;
	h)	help="$OPTARG" 
		hflg=y
		;;
	e)	serror="$OPTARG" 
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

items="$*"

if [ $errflg ]
then
	printf "usage: ckitem [-Q] [-W width] [-uno] [-f filename] [-l label] [[-i invis][,...]] [-m max] [-d default] [-h help] [-e error] [-p prompt] [-k pid[-s signal]][choice[...]]\n" >&2
	exit 2
fi

if [ $fflg ]
then
	[ -r $file ] || { echo $prog: ERROR can\'t open $file >&2; exit 1; }
else
	[ $# -eq 0 ] && exit 4
fi

if [ ! "$nflg" ]
then
	for item in $* $invis
	do
		echo $item
	done | cat $file - | sort -u > /tmp/$prog.$$
else
	for item in $* $invis
	do
		echo $item
	done | cat $file - > /tmp/$prog.$$
fi

IFS="	"
while true
do
	if [ ${i:-0} -eq 0 -o -n "$display" ]
	then
		[ $lflg ] && printf "\n%s\n" "$label" >&2
		if [ ! "$uflg" ]
		then
			while read item description
			do
				[ ${#item} -gt ${pad:=0} ] && pad=${#item}
			done < /tmp/$prog.$$
		fi
		i=0
		while read item description
		do
			(( i += 1 ))
			items[$i]=$item
			[ $uflg ] && printf "%s  %s\n" $item "$description" >&2 \
					  || printf "%3d  %-${pad}s     %s\n" $i $item "$description" >&2
		done < /tmp/$prog.$$
		display=
	fi

	echo "$prompt $values${default:+ default '$default'}: \c" >&2
	prev="$ans"
	read ans

	[ -z "$ans" -a "$dflg" ] && { echo $default; exit 0; }

	case "$ans" in
	[0-9]|[0-9][0-9])
		if [ ! "$uflg" ]
		then
			if [ "$ans" -ge 1 -a "$ans" -le $i ]
			then
				echo ${items[$ans]}
				exit 0
			fi
		fi
		printf "$error\n" >&2
		;;
	\?\?)	
		printf "$help\n" >&2
		display=y
		;;
	\?)	
		if [ "$prev" = \? ]
		then
			display=y
		else
			printf "$help\n" >&2
		fi
		;;
	[Qq])	
		[ $Qflg ] && { printf "$error\n"; continue; }
		[ $kflg ] && kill -${signal:-15} $pid
		return 3
		;;
	""|"[ 	]*")	printf "$error\n" >&2
		;;
	*)	match=0 matched=
		while read item description
		do
			case $item in
			$ans*)	(( match += 1 ))
					matched="$item" ;;
			esac
		done < /tmp/$prog.$$
		if [ "$match" -eq 1 ]
		then
			echo $matched
			exit 0
		fi
		printf "$serror\n" >&2
	esac
done
