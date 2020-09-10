#ident $Header$
################################################################################
#
# Get apps password
#
################################################################################

appspass()
{
	[ $# -ne 1 ] && { echo "usage: $prog <path to apps context file>" >&2; return 2; }
	[ -r "$1" ] || { echo "$prog: cannot access $1" >&2; return 1; }

	if ident $1 | grep -q 'Header: adxmlctx'
	then
		# e.g. /u20/app/DINO/apps/admin/DINO_ukgtjd35.xml
		appctxfile=`realpath $1`
	else
		echo "$prog: $1 is not a valid application node context file" >&2
		return 1
	fi

	_s_dbSid=`xmlvar s_dbSid $appctxfile`
	_s_weboh_oh=`xmlvar s_weboh_oh $appctxfile`

	if [ -r $_s_weboh_oh/Apache/modplsql/cfg/wdbsvr.app ]
	then
		awk '
		/\[DAD_'$_s_dbSid']/,/;/ {
			if ($1 == "username")
				username = tolower($3)
			if ($1 == "password")
				password = tolower($3)
		}
		END {
			#print username "/" password
			print password
		}' $_s_weboh_oh/Apache/modplsql/cfg/wdbsvr.app

	fi
}
typeset -fx appspass
