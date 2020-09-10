# suboavars()
#  replace context variables in file or stdin with context values taken from context
#  file
suboavars()
{
	typeset opt= vflg= fflg= dflg=0 tflg=0 ctxfile= vals= file= \
		usage="usage: suboavars [-d|-t] [-v <var>=<val>...] -f <contextfile> [<file>]"

	while getopts f:v:dt opt 2>&-
	do
		case $opt in
		f)	ctxfile="$OPTARG"
			fflg=y
			;;
		v)	vals="$vals $OPTARG"
			vflg=y
			;;
		d)	[ $tflg -eq 1 ] && errflg=y
			dflg=1
			;;
		t)	[ $dflg -eq 1 ] && errflg=y
			tflg=1
			;;
		\?)	errflg=y ;;
		esac
	done
	shift `expr $OPTIND - 1`

	[ $# -gt 0 ] && errflg=y
	[ $fflg ] || errflg=y
	[ $errflg ] && { print -u2 "$usage"; return 2; }

	[ $# -eq 1 ] && file=$1

	awk -v dflg=$dflg -v ctxfile=$ctxfile '
		BEGIN {
			if (dflg) {	# driver file syntax
				lb = "<"; rb = ">"
			} else {
				lb = "%"; rb = "%"
			}
			pattern = sprintf("%c[cs]_[a-zA-Z0-9_]+%c", lb, rb)
		}
		/[<%][cs]_/ {
			line = $0
			split("", vars, ":")	# clear array

			while (match(line, pattern)) {
				s = substr(line, RSTART+1, RLENGTH-2)
				vars[n++] = s
				line = substr(line, 1 + RSTART + RLENGTH)
			}
			for (var in vars) {
				val = ""
				for (i = 0; i < ARGC; i++) {
					v = substr(ARGV[i], 1, index(ARGV[i], "=")-1)
					if (v == vars[var])	# override context file 
						val = substr(ARGV[i], index(ARGV[i], "=")+1)
						#printf("COMPARE %s: %s to %s\n", ARGV[i], v, vars[var])
				}
				if (length(val) == 0) {
					cmd = sprintf("ctxvar -if %s %s", vars[var], ctxfile)
					cmd | getline val
				}
				gsub(lb vars[var] rb, val)
			}
		}
		{ print }' $vals $file
}
typeset -fx suboavars
