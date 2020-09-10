#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
prefix="/opt/oracle"

if [ -t ]
then
	stty erase "^?" kill "^U" werase "^W" intr "^C" eof "^D" susp "^Z" echoke
	stty hupcl ixon ixoff
fi

PATH=$prefix/bin:$prefix/sbin:/usr/bin:$HOME/bin/usr/ccs/bin:/usr/sbin:/usr/local/bin
case `uname -s` in
AIX)	PATH=$PATH:/opt/freeware/bin ;;
HP-UX)	PATH=$PATH:/usr/contrib/bin:/opt/langtools/bin:/opt/perf/bin 
		eval `grep -s '^ORA_CRS_HOME=' /etc/cmcluster/*/oc.conf`   # set the last found
		[ "$ORA_CRS_HOME" ] && export ORA_CRS_HOME
esac
export PATH

MANPATH=$MANPATH:$prefix/share/man:$HOME/share/man
export MANPATH

umask 022

nodename=`uname -n`
PS1=`id -un`@${nodename%%.*}:\ 
export PS1

if [ "$SHELL" = "bash" ]
then
	export BASH_ENV=~/.shrc
else
	ENV=~/.shrc export ENV
fi

if [ -t ] 
then
	case $0 in
	-*) [ -r $HOME/.issue ] && cat $HOME/.issue
	esac
fi

