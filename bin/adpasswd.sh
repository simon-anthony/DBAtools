#!/usr/bin/sh -
#ident $Header$
# vim: syntax=sh:sw=4:ts=4:
################################################################################
# adwlspasswd: extract WLS passwords
#
################################################################################
# Copyright (C) 2008 Simon Anthony
# 
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License or, (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program. If not see <http://www.gnu.org/licenses/>>
# 
PATH=/bin:/usr/bin:BINDIR:SBINDIR export PATH
FPATH=BASEDIR/share/lib/functions export FPATH
unset ORACLE_HOME ORACLE_SID TWO_TASK

prog=`basename $0`
usage="[-a|-w] [-u] -s <s_systemname>"

ssh="ssh -o BatchMode=yes -o ChallengeResponseAuthentication=no"

while getopts s:uwa opt 2>&-
do
	case $opt in
	a)	aflg=y			# decrypt apps password
		[ $wflg ] && errflg=y
		;;
	w)	wflg=y			# decrypt weblogic password
		[ $aflg ] && errflg=y
		;;
	s)	s_systemname="$OPTARG"
		sflg=y
		;;
	u)	uflg=y			# return user name
		;; 				# - usually "apps" for -a and "weblogic" for -w
	\?)	errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ ! "$sflg" ] && errflg=y
[ $# -ne 0 ] && errflg=y

[ $errflg ] && { echo "usage: $prog $usage" >&2; exit 2; }

[ ! "$aflg" -a ! "$wflg" ] && aflg=y	# -a default if neither -a or -w

if ! appctxfile=`getctx -a -s $s_systemname 2>&-`
then
	print -u2 "$prog: cannot open context file for '$s_systemname'"
	exit 1
fi

for var in s_fmw_home s_dbSid
do
	eval `ctxvar -ifv $var $appctxfile`
done

domain_home=$s_fmw_home/user_projects/domains/EBS_domain_$s_dbSid

if [ -r $domain_home/bin/setDomainEnv.sh ]
then 
	. $domain_home/bin/setDomainEnv.sh
else
	print -u2 "$prog: cannot locate 'DOMAIN_HOME/bin/setDomainEnv.sh'"
	exit 1
fi

if [ $aflg ]
then
	if [ -r $domain_home/config/jdbc/EBSDataSource-[0-9]*-jdbc.xml ]
	then
		if [ $uflg ]
		then
			sed -n '/<name>user<\/name>/ {
				n;
				s;.*<value>\(.*\)</value>.*;\1;
				p;
				q;
			}' $domain_home/config/jdbc/EBSDataSource-[0-9]*-jdbc.xml
			exit
		fi
		hash=`sed -n '/password-encrypted/ {
			s;.*<password-encrypted>\(.*\)</password-encrypted>;\1;p; }' \
			$domain_home/config/jdbc/EBSDataSource-[0-9]*-jdbc.xml`
	else
		print -u2 "$prog: cannot open 'DOMAIN_HOME/config/jdbc/EBSDataSource-[0-9]*-jdbc.xml'"
		exit 1
	fi
else
	if [ -r $domain_home/servers/AdminServer/security/boot.properties ]
	then
		if [ $uflg ]
		then
			hash=`sed -n '/username=/ { s;[^=]*=;;p;  }' $domain_home/servers/AdminServer/security/boot.properties`
		else
			hash=`sed -n '/password=/ { s;[^=]*=;;p;  }' $domain_home/servers/AdminServer/security/boot.properties`
		fi
	else
		 print -u2 "$prog: cannot open 'DOMAIN_HOME/servers/AdminServer/security/boot.properties'"
		 exit 1
	fi
fi

tmpfile=`mktemp -c`

cat <<-! > $tmpfile
	from weblogic.security.internal import *
	from weblogic.security.internal.encryption import *

	encryptionService = SerializedSystemIni.getEncryptionService(".")
	clearOrEncryptService = ClearOrEncryptedService(encryptionService)

	hash  = "$hash"

	print "Plaintext: " + clearOrEncryptService.decrypt(hash)
!

java weblogic.WLST $tmpfile | sed  -n '/Plaintext/ { s;[^:]*: ;;p;  }'

rm -f $tmpfile
