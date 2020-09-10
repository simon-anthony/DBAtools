# $Header: DBAtools/trunk/share/lib/functions/suboavar.sh 187 2017-11-16 15:25:59Z xanthos $
# suboavar - substitute s_ or c_ variable from a file
#   $1  - variable
#   $2  - value
#  [$3] - file - or stdin
suboavar()
{
	awk '/^[^#].*%[a-zA-Z0-9_]*%/ {
		gsub(%'"$1"', '"$2"', $0)
	}' $1
}
typeset -fx suboavar
