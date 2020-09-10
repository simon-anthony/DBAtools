#!/bin/ksh -
# $Header$
: ${ORACLE_HOME:=/opt/oracle/product/grid}
LD_LIBRARY_PATH=$ORACLE_HOME/lib
PATH=/usr/bin:/usr/sbin:$ORACLE_HOME/bin
export ORACLE_HOME LD_LIBRARY_PATH PATH

while getopts -a asmcmd "vpo" opt 
do
	case $opt in
	v)	vflg=y ;;	# add a VTOC with s0
	p)	pflg=y ;;	# provision the disks
	o)	oflg=y ;;	# change ownership
	?) errflg=y ;;
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 1 ] && vol="$1"
[ $# -gt 1 ] && errflg=y

if [ $errflg ]
then
	print -u2 "usage: asmdisk [-vpo] [volume]" 
	print -u2 "  -v add a VTOC with s0"
	print -u2 "  -p provision the disk"
	print -u2 "  -o change ownership of the disk"
	exit 2
fi

if [ -x $ORACLE_HOME/bin/kfod ] 
then
	ex -s $ORACLE_HOME/bin/kfod <<-!
		g/OHOME=/s;=.*;=$ORACLE_HOME;
		w!
	!
else
	if [ "X$ORACLE_HOME" = "X" ]
	then
		print -u2 "asmdisk: ORACLE_HOME not set"
		exit 1
	fi
	print -u2 "asmdisk: cannot access ORACLE_HOME/bin/kfod"
	exit 1
fi

fmt() {
	OPTIND=1
	typeset fflg= nflg= label= errflg=
	while getopts "fn:" opt
	do
		case $opt in
		f)	fflg=y ;; # format
		n)	lfgl=y
			label=$OPTARG ;;
		\?)	errflg=y
		esac
	done
	shift `expr $OPTIND - 1`

	[ $# -eq 1 ] || errflg=y
	[ $errflg ] && { print -u2 "fmt: invalid flag"; return 2; }

	if [ $fflg ]
	then
		: format 
	fi
	sudo prtvtoc $1 |
	nawk '
	$3 == "sectors/cylinder" { 
		sectors_cylinder = $2
	}
	$1 == "*" { next }
	{
		line[$1] = $0
	}
	END {
		$0 = line[2]

		# partition  0
		first = sectors_cylinder
		last = $6
		count = last - first + 1
		printf "%8d %6d    %02d %10d %9d %9d\n", 0, 1, 1, first, count, last

		# partition 8
		first = 0
		count = sectors_cylinder
		last = count - 1
		printf "%8d %6d    %02d %10d %9d %9d\n", 8, 1, 1, first, count, last

		print line[2]
	}' > /tmp/vtoc.$$
	fmthard ${nflg:+-n $label} -s /tmp/vtoc.$$ ${1%s2}s0
}

sudo iscsiadm list target -S | 
	nawk '
	/Target:/ { target = substr($(NF),match($(NF), /[^\.]+$/)) }
	/OS Device/ { print $(NF) " # " target }' | sort -k2 | 
while read path sep volume
do
	[  -n "$vol" -a "X$vol" != "X$volume" ] && continue
	[ $nl ] && echo
	echo $volume: $path 
	case $volume in
	asm[0-9]|asm[0-9][0-9]|gimr[0-9])
		# Need ~ 40GB for GIMR
		[ $vflg ] && fmt -n $volume $path
		[ $oflg ] && {
			sudo chown grid:asmadmin ${path%s2}s0
			sudo chown -h grid:asmadmin ${path%s2}s0 # change symlink owner also
			sudo chmod 660 ${path%s2}s0
		}
		# ASM Filter Driver - Provision the disks
		[ $pflg ] && {
			dd if=/dev/zero of=${path%s2}s0 bs=4k count=1000
			asmcmd afd_label $volume ${path%s2}s0 --init
		}
		sudo prtvtoc $path | sed -n '/map$/ { p; }; /Partition/,$ { p; }'
		kfod asm_=${path%s2}s0 label=TRUE status=TRUE | grep '^ *[0-9]:'
		if [ -h ${path%s2}s0 ] 
		then
			ls -l ${path%s2}s0  | sed 's;->.*;->;'
			ls -l ${path%s2}s0 | awk '{ print "/dev/rdsk/" $(NF) }' | xargs -n1 realpath | xargs ls -l
		else
			ls -l ${path%s2}s0 
		fi
		;;
	ocr*)
		# OCR/Voting disk is not provisioned for ASM
		[ $vflg ] && fmt -n $volume $path
		[ $oflg ] && {
			sudo chown grid:asmadmin ${path%s2}s0
			sudo chown -h grid:asmadmin ${path%s2}s0 # change symlink owner also
			sudo chmod 660 ${path%s2}s0
		}
		[ $pflg ] && dd if=/dev/zero of=${path%s2}s0 bs=4k count=1000
		sudo prtvtoc $path | sed -n '/map$/ { p; }; /Partition/,$ { p; }'
		if [ -h ${path%s2}s0 ] 
		then
			ls -l ${path%s2}s0  | sed 's;->.*;->;'
			ls -l ${path%s2}s0 | awk '{ print "/dev/rdsk/" $(NF) }' | xargs -n1 realpath | xargs ls -l
		else
			ls -l ${path%s2}s0 
		fi
	esac
	nl=y
done

