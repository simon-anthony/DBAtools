#ident $Header: DBAtools/trunk/share/lib/functions/xmlvar.sh 114 2017-08-22 09:08:43Z xanthos $
# xmlvar: return value of xml variable in context file
#  - really should use a parser
#   $1 - variable name  $2 - XML file
#
xmlvar()
{
	[ $# -eq 2 ] || {
		echo "usage: xmlvar <variable name> <XML context file>" >&2;
		return 1; }
	[ -r "$2" ] || { echo "xmlvar: cannot open $2" >&2; return 1; }

	_ctx=`ident $2 2> /dev/null | awk '
		/Header:/ { print substr($2, 1, match($2, /\./)-1); exit }'`

	case $_ctx in
	adxdbctx) : ;; # Apps context file
	adxmlctx) : ;; # DB context file
	*)	echo "xmlvar: invalid XML context file '$2'"; return 1;
	esac

	sed -n 's;.*[ 	]\{1,\}oa_var="'$1'"[^>]*>\(.*\)<.*;\1;p' $2
}
typeset -fx xmlvar
