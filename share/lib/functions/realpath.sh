#ident $Header$
# realpath: canonicalize the supplied path
#
realpath()
{
	[ $# -eq 1 ] || { echo "usage: realpath <path>" >&2; return 1; }

        if expr "$1" : '.*/.*' >/dev/null 2>&1  # has directory component
        then
                dir=`dirname $1`
                ( cd $dir 2>&- && echo `pwd`/`basename $1` )
        else                                    # just filename
                echo `pwd`/$1
	fi
}
typeset -fx realpath
