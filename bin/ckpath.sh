#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:

PATH=/usr/bin export PATH

Qflg= Wflg= dflg= hflg= eflg= hflg= kflg= sflg= errflg= aflg= lflg= bflg= cflg= fflg= yflg= nflg= oflg= zflg= rflg= tflg= wflg= xflg= 

default=
prompt="\nEnter a pathname"
values="[?,q]"
error="\tERROR: A pathname is a filename, optionally preceeded by parent directories."
help="\tA pathname is a filename, optionally preceeded by parent directories."

pid=
signal=

trap "exit 1" 1 2 3 15

while getopts QW:albcfynozrtwxd:h:e:p:k:s: opt 2>&-
do
	case $opt in
	Q)	values="[?]"		# do not allow 'q'
		Qflg=y
		;;
	W)	Wflg=y
		;;
	a)	aflg=y					# pathname must be absolute
		[ $lflg ] && errflg=y
		;;
	l)	lflg=y					# ...      must be relative
		[ $aflg ] && errflg=y
		;;
	b)	bflg=y					# pathname must be block special file
		[ "$cflg" -o "$fflg" -o "$yflg" ] && errflg=y
		bcfy=b
		;;
	c)	cflg=y					# ...      must be character special file
		[ "$bflg" -o "$fflg" -o "$yflg" ] && errflg=y
		bcfy=c
		;;
	f)	fflg=y					# ...      must be regular file
		[ "$cflg" -o "$bflg" -o "$yflg" ] && errflg=y
		bcfy=f
		;;
	y)	yflg=y					# ...      must be directory
		[ "$cflg" -o "$fflg" -o "$bflg" ] && errflg=y
		bcfy=d
		;;
	n)	nflg=y					# pathname must not exist (new)
		[ "$oflg" -o "$zflg" ] && errflg=y
		noz=n
		;;
	o)	oflg=y					# ...      must exist (old)
		[ "$nflg" -o "$zflg" ] && errflg=y
		noz=o
		;;
	z)	zflg=y					# ...      must exist (>zero)
		[ "$oflg" -o "$nflg" ] && errflg=y
		noz=z
		;;
	r)	rflg=y					# pathname must be readable
		rtwx="r $rtwx"
		;;
	w)	wflg=y					# ...      must be writable
		rtwx="w $rtwx"
		;;
	t)	tflg=y					# ...      must be touchable
		rtwx="t $rtwx"
		;;
	x)	xflg=y					# ...      must be executable
		rtwx="x $rtwx"
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
	\?)	errflg=?
	esac
done
shift `expr $OPTIND - 1`

[ "$sflg" -a ! "$kflg" ] && errflg=y

if [ $errflg ]
then
	# printf "usage: ckpath [-Q] [-W width] [-a|l] [-b|c|f|y] [-n[o|z]] [-rtwx] [-d default] [-h help] [-e error] [-p prompt] [-k pid[-s signal]]\n" >&2

	if [ $errflg = \? ]
	then
		printf "usage: ckpath [options] [[-a|l][b|c|f|y][n|[o|z]][rtwx]\n"
		printf "\t-a  #absolute path\n"
		printf "\t-b  #block special device\n"
		printf "\t-c  #character special device\n"
		printf "\t-f  #ordinary file\n"
		printf "\t-l  #relative path\n"
		printf "\t-n  #must not exist (new)\n"
		printf "\t-o  #must exist (old)\n"
		printf "\t-r  #read permission\n"
		printf "\t-t  #permission to create (touch)\n"
		printf "\t-w  #write permission\n"
		printf "\t-x  #execute permission\n"
		printf "\t-y  #directory\n"
		printf "\t-z  #non-zero length\n"
		printf "where options may include\n"
		printf "\t-Q  #quit not allowed\n"
		printf "\t-W width\n"
		printf "\t-d default\n"
		printf "\t-e error\n"
		printf "\t-h help\n"
		printf "\t-k pid [-s signo]\n"
		printf "\t-p prompt\n"
	else
		printf "ckpath: ERROR: mutually exclusive options used\n"
	fi
	exit 2
fi

if [ ! "$pflg" ]
then
	[ $aflg ] && prompt="Enter an absolute pathname"
	[ $lflg ] && prompt="Enter a relative pathname"
fi

while true
do
	#echo "$prompt $values${default:+ default '$default'}: \c" >&2
	echo "$prompt $values: \c" >&2
	read ans

	[ -z "$ans" -a "$dflg" ] && ans="$default"

	fail=

	case "$ans" in
	\?)	
		printf "$help\n"
		continue
		;;
	[Qq])	
		[ $Qflg ] && { printf "$error\n"; continue; }
		[ $kflg ] && kill -${signal:-15} $pid
		printf "$ans"
		return 3
		;;
	"")	printf "\tERROR: Input is required.\n" >&2
		continue
		;;
	*)	expr //"$ans" : '//[^ ]*$' >&- || {
			printf "\tERROR: Pathname does not meet suggested syntax standard.\n" >&2
			continue
		}
		if [ $aflg ]
		then
			expr //"$ans" : '///' >&- || {
				printf "\tERROR: Pathname must begin with a slash (/).\n" >&2
				continue
			}
		fi
		if [ $lflg ]
		then
			expr //"$ans" : '///' >&- && {
				printf "\tERROR: Pathname must not begin with a slash (/).\n" >&2
				continue
			}
		fi
		if [ $bcfy ] 
		then
			if ls $ans >/dev/null 2>&1 | grep -q 'not found'
			then
				:
				case $bcfy in
				b)	[ -$bcfy $ans ] || {
						printf "\tERROR: Pathname must specify a block special device.\n" >&2
						continue
					}
					;;
				c)	[ -$bcfy $ans ] || {
						printf "\tERROR: Pathname must specify a character special device.\n" >&2
						continue
					}
					;;
				f)	[ -$bcfy $ans ] || {
						printf "\tERROR: Pathname must be a regular file.\n" >&2
						continue
					}
					;;
				y)	[ -$bcfy $ans ] || {
						printf "\tERROR: Pathname must specify a directory.\n" >&2
						continue
					}
				esac
			fi
		fi
		if [ "$rtwx" ]
		then
			for perm in $rtwx
			do
				if [ $perm = t ]
				then
					if ls $ans >/dev/null 2>&1 | grep -q 'not found'
					then
						echo YES
						if [ -d `basename "$ans"` -a -w `basename "$ans"` ]
						then
							:
						else
							printf "\tERROR: Pathname cannot be created.\n" >&2
							fail=y
							continue
						fi
					else
						touch "$ans" 2>&- || {
							printf "\tERROR: Pathname cannot be created.\n" >&2
							fail=y
							continue
						}
					fi
				elif [ ! "$nflg" ]
				then
					case $perm in
					r) [ -$perm "$ans" ] || {
							printf "\tERROR: Pathname is not readable.\n" >&2
							fail=y
							continue
						}
						;;
					w) [ -$perm "$ans" ] || {
							printf "\tERROR: Pathname is not writable.\n" >&2
							fail=y
							continue
						}
						;;
					x) [ -$perm "$ans" ] || {
							printf "\tERROR: Pathname is not executable.\n" >&2
							fail=y
							continue
						}
					esac
				fi
			done
		fi
		if [ "$noz" ]
		then
			case "$noz" in
			n)	ls "$ans" >/dev/null 2>&1 | grep -q 'not found' || {
					printf "\tERROR: Pathname must not already exist.\n" >&2
					continue
				}
				;;
			o)	ls "$ans" >/dev/null 2>/dev/null || {
					printf "\tERROR: Pathname does not exist.\n" >&2
					continue
				}
				;;
			z)	ls "$ans" >/dev/null 2>/dev/null && {
					[ -s "$ans" ] || {
						printf "\tERROR: Pathname must be a file of non-zero length.\n" >&2
						continue
					}
				}
			esac
		fi
	esac

	if [ ! "$fail" ]
	then
		printf "%s" "$ans"
		return 0
	fi
done
