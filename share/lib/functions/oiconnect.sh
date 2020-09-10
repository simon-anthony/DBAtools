#ident $Header$
# oiconnect: test connection to oracle
#  - really should use a parser
#   $1 - username/passwd
#
oiconnect()
{
	expr "$1" : '[^/]\{1,\}/[^/]\{1,\}' \| "$1" : '/[ 	]\{1,\}[Aa][Ss][ ]\{1,\}[A-Za-z]\{1,\}' >/dev/null || {
		echo usage: oiconnect username/password >&2
		return 2
	}
	[ $ORACLE_HOME ] || {
		echo oiconnect: ORACLE_HOME not set >&2; return 2; }
	[ -z "$ORACLE_SID" -a -z "$TWO_TASK" ] && {
		echo oiconnect: ORACLE_SID or TWO_TASK must be set >&2; return 1; }
	[ -x $ORACLE_HOME/bin/sqlplus ] || {
		echo oiconnect: sqlplus not found >&2; return 1; }

	$ORACLE_HOME/bin/sqlplus -s <<-! >/dev/null 2>&1
		$1
		whenever sqlerror exit failure
		whenever oserror exit failure
		SELECT 1 from DUAL;
		exit success
	!
	return $?
}
typeset -fx oiconnect
