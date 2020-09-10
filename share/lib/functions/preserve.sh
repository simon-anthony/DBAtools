#ident $Header$
################################################################################
#
# Make a backup copy of $1 - appending 1, 2, 3 ...
#
################################################################################
preserve()
{
	[ "$1" ] || return 2
	[ -f "$1" ] || { echo preserve: $1 non-existent >&2; return 1; }
	_i=
	if [ -f $1.bak ]
	then
		if [ -f $1.bak.1 ]
		then
			_i=`ls $1.bak.?([0-9]*) |
			sed 's;.*\.\([0-9]\{1,\}\)$;\1;p'| sort -nr | sed 1q`
			_i=`expr $_i + 1`
		else
			_i=1
		fi
	fi
	cp $1 $1.bak${_i:+.$_i}
}
typeset -fx preserve
