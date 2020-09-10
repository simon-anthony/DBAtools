# $Header$
# getoavars - extract s_ variables from a file
getoavars()
{
	for file
	do
		awk '/^[^#].*%[a-zA-Z0-9_]*%/ {
			while (index($0, "%") != 0) {
				$0 = substr($0, index($0, "%")+1)
				s = sprintf("%s", substr($0, 1, index($0, "%")-1))
				if (match(s, /^s_/))
					print s
				
				$0 = substr($0, index($0, "%")+1)
			}
		}' $file
	done | sort -u
}
typeset -fx getoavars
