#!/usr/bin/sh -
#ident $Header$
# vim:syntax=sh:sw=4:ts=4:
#
PATH=/usr/bin:BINDIR
FPATH=BASEDIR/share/lib/functions export FPATH
prog=`basename $0`
 
usage="[-v[-x]] [-h] [-p <password> [-i]] [[-d|-a|-o] -s <s_systemname>] [-f <file>] <hosts...>"
conn_errs=
 
while getopts vxhif:p:daos: opt 2>&-
do
	case $opt in
	f)	fflg=y                        # output file
		file=$OPTARG
		;;
	p)	pflg=y                        # password
		password=$OPTARG
		;;
	s)	ORACLE_SID="$OPTARG"          # ORACLE_SID
		export ORACLE_SID
		sflg=y
		;;
	d)	dflg=y                        # use db env file
		[ $aflg ] && errflg=y
		;;
	a)	aflg=y                        # use apps env file
		[ $dflg ] && errflg=y
		;;
	o)	oflg=y                        # use oratab
		;;
	v)	vflg=y                        # verbose
		;;
	x)	xflg=y                        # extended ps(1)
		;;
	h)	hflg=y                        # test remote host connection
		;;
	i)	iflg=y                        # install types
		;;
	\?)   errflg=y
	esac
done
shift `expr $OPTIND - 1`

[ \( -n "$dflg" -o -n "$aflg" -o -n "$oflg" \) -a ! "$sflg" ] && errflg=y
											# -s mandatory if -a, -d or -o
[ "$iflg" -a ! "$pflg" ] && errflg=y		# -p mandatory if -i
[ "$xflg" -a ! "$vflg" ] && errflg=y		# -v mandatory if -x

[ $# -ge 1 ] || errflg=y					# at least one host name required
 
[ $errflg ] && { echo usage: $prog "$usage" >&2; exit 2; }

# Context file(s)
#
if [ $aflg ]
then
	appctxfile=`getctx -a -s $ORACLE_SID` || exit $?
fi

if [ $dflg ]
then
	dbctxfile=`getctx -d -s $ORACLE_SID` || exit $?
fi

if [ $oflg ]
then
	oratab=`ls /var/opt/oracle/oratab /etc/oratab 2>&-`
	[ $oratab ] || { echo "$prog: cannot find oratab" >&2; exit 1; }
	ORACLE_HOME=`awk -F: '$1 == "'$ORACLE_SID'" { print $2 }' $oratab`
	[ $ORACLE_HOME ] || {
		echo "$prog: cannot find '$ORACLE_SID' in $oratab" >&2; exit 1; }
	[ $ORACLE_HOME ] || { echo $prog: ORACLE_HOME not set >&2; exit 1; }
	export ORACLE_HOME
	PATH=$ORACLE_HOME/bin:$PATH export PATH
fi

if [ ! "$aflg" -a ! "$dflg" -a ! "$oflg" ]
then
	[ $ORACLE_HOME ] || { echo $prog: ORACLE_HOME not set >&2; exit 1; }
	PATH=$ORACLE_HOME/bin:$PATH export PATH
fi

################################################################################

if [ $dbctxfile ]
then
	s_dbSid=`ctxvar -if s_dbSid $dbctxfile`	# ORACLE_SID
	s_db_local=`ctxvar -if s_db_local $dbctxfile`
	s_dbGlnam=`ctxvar -if s_dbGlnam $dbctxfile`
	s_db_oh=`ctxvar -if s_db_oh $dbctxfile`
	s_db_home_file=`ctxvar -if s_db_home_file $dbctxfile`
fi
if [ $appctxfile ]
then
	s_dbSid=`ctxvar -if s_dbSid $appctxfile`	# ORACLE_SID
	s_tools_twotask=`ctxvar -if s_tools_twotask $appctxfile`
	s_dbGlnam=`ctxvar -if s_dbGlnam $appctxfile`
	s_tools_oh=`ctxvar -if s_tools_oh $appctxfile`
	s_tools_tnsadmin=`ctxvar -if s_tools_tnsadmin $appctxfile`
	s_tools_home_file=`ctxvar -if s_tools_home_file $appctxfile`

	[ $pflg ] || password=`getappspass -s $s_systemname`
fi

trap "if [ A"$password" = "A" ]; then [ -t ] && stty echo; fi" 1 2 3 15

if [ A"$password" = "A" ]
then
      echo "Enter APPS password: \c"
      [ -t ] && stty -echo
      read password
      echo
      [ -t ] && stty echo
fi

if [ $s_dbSid ]
then
	if [ $dbctxfile ]
	then
		(
		if ORACLE_HOME=$s_db_oh ORACLE_SID=$s_db_local oiconnect apps/$password
		then
			exit 0
		else
			echo $prog: ERROR - user apps cannot connect - check appspass >&2
			echo Check listener is up >&2
			exit 1
		fi ) || exit 1
		. $s_db_home_file
	elif [ $appctxfile ]
	then
		(
		if ORACLE_HOME=$s_tools_oh TNS_ADMIN=$s_tools_tnsadmin \
			ORACLE_SID= TWO_TASK=$s_tools_twotask oiconnect apps/$password
		then
			exit 0
		else
			echo $prog: ERROR - user apps cannot connect - check appspass >&2
			echo Check connection with SQL\*Plus, if ORA-12505 it may be that TNS configuration for RAC has not yet been generated. >&2
			exit 1
		fi ) || exit 1
		. $s_tools_home_file
	else
		echo $prog: cannot determine context file >&2
		exit 1
	fi
else
	if [ ! "$TNS_ADMIN" ]
	then
		if [ ! "$ORACLE_SID" ]
		then
			echo $prog neither TNS_ADMIN nor ORACLE_SID not set >&2
			exit 1
		fi
	fi
	if oiconnect apps/$password
	then
		:
	else
		echo $prog: ERROR - user apps cannot connect - check appspass >&2
		echo Check connection with SQL\*Plus, if ORA-12505 it may be that TNS configuration for RAC has not yet been generated. >&2
		exit 1
	fi
fi

if [ $fflg ]
then
      > $file || { echo "$prog: cannot create '$file'" >&2; exit 1; }
fi

# Parse hosts list
# e.g. turn
#  ukgtja05 ukgtja06 ukgtja07 ukgtja08 ukgtja09 ukgtja10 ukgtja19
# into
#  'ukgtja05', 'ukgtja06', 'ukgtja07', 'ukgtja08', 'ukgtja09', 'ukgtja10', 'ukgtja19'
for machine
do
      [ "$machines" ] && machines="$machines, '$machine'" || machines="'$machine'"
      if [ $hflg ]
      then
          if ssh $machine : 2>/dev/null
          then
              echo $prog: ssh connection test to $machine OK
          else
              echo $prog: ssh connection test to $machine FAILED
              conn_errs=y
          fi
      fi
done

[ $conn_errs ] && exit 1

: ${TMPDIR:=/tmp}
export TMPDIR
if [ $iflg ]
then
	echo $prog: initialising for first use
	if sqlplus -s <<-! > /tmp/$prog$$.out
		  apps/$password
		  whenever sqlerror exit failure
		  whenever oserror exit failure

		  @BASEDIR/share/lib/sql/ConcatStr.sql
	!
	then
		exit 0
	else
		echo $prog: error in SQL command >&2
		cat /tmp/$prog$$.out
		exit 1
	fi
fi
 
if sqlplus -s <<-! > /tmp/$prog$$.out
      apps/$password
      -- 
      set lines 32767 pages 0 feedback off trimspool on heading off long 40000 longchunksize 40000
 
      column server_program   format a16    
      column spid             format a6 
      column machine          format a16   
      column client_program   format a80
 
      clear breaks computes
 
      break on row skip 1

      whenever sqlerror exit sql.sqlcode
 
      SELECT      'echo "\n' ||
                  server_program ||' ['||COUNT(spid)||']' || ' <- ' || machine || ' ['||COUNT(DISTINCT process)||'/'||COUNT(process)||']' ||
                  '"; ' ||
                  'ssh ' || machine || ' "BINDIR/ps -${xflg:+x}p ' ||
                  CONCATSTR(process) || ' -o pid=PROCESS -o vsz,args=COMMAND |
                         nl -w3 -v0 -s\ "'
      FROM (
            SELECT server_program,
                   spid,
                   machine,
                   process,
                   RANK() OVER (
                         PARTITION BY server_program, spid, machine, process
                         ORDER BY seq DESC )  AS rank
            FROM (
                  SELECT CASE WHEN p.program LIKE '%TNS%'
                              THEN SUBSTR(p.program,
                                      INSTR(p.program, '@')+1,
                                      INSTR(p.program, ' ')-INSTR(p.program, '@')-1)
                              ELSE SUBSTR(p.program,
                                      INSTR(p.program, '@')+1) END               AS  server_program,
                         p.spid                                                  AS  spid,
                         DECODE(INSTR(machine, '.'),
                                     0, machine,
                                        SUBSTR(s.machine,
                                           1, instr(s.machine, '.') - 1))        AS  machine,
                         s.process                                               AS  process,
                         s.fixed_table_sequence                                  AS  seq
                  FROM   gv\$process p, gv\$session s
                  WHERE  p.addr = s.paddr
                  AND    p.inst_id = s.inst_id
                  AND    s.type != 'BACKGROUND'
                  AND    NVL(s.program, 'Forms and Other TNS') NOT LIKE '% (QMN%'
                  AND    NVL(s.program, 'Forms and Other TNS') NOT LIKE '% (CJQ%'
                  AND    NVL(s.program, 'Forms and Other TNS') NOT LIKE '% (P%'
                  AND    NVL(s.program, 'Forms and Other TNS') NOT LIKE '% (J%'
                  AND    p.program NOT LIKE '% (QMN%'
                  AND    p.program NOT LIKE '% (CJQ%'
                  AND    p.program NOT LIKE '% (P%'
                  AND    p.program NOT LIKE '% (J%'
                  AND  ( DECODE(INSTR(machine, '.'),
                              0, machine,
                                 SUBSTR(s.machine, 1, instr(s.machine, '.') - 1))
                              IN (
                                    SELECT host_Name FROM gv\$instance)
                        OR
                          DECODE(INSTR(machine, '.'),
                              0, machine,
                                 SUBSTR(s.machine, 1, instr(s.machine, '.') - 1))
                              IN ($machines)
                  )
                  AND   s.process IS NOT NULL     -- not all programs/platforms register PID
            )
      )
      WHERE rank = 1
      AND machine IN ($machines)
      GROUP BY server_program, machine
      ORDER BY server_program, machine
 
      set termout off
      spool $TMPDIR/$prog$$.sh
      /
      spool off
      host sh $TMPDIR/$prog$$.sh > $TMPDIR/$prog$$.lst
      set termout on

      exit success
!
then
      [ -r $TMPDIR/$prog$$.lst ] || { echo $prog: failed to produce report >&2; exit 1; }
      [ "$fflg" -a "$vflg" ] && cat $TMPDIR/$prog$$.lst > $file
else
      echo $prog: error in SQL command >&2
	  cat /tmp/$prog$$.out
      exit 1
fi
 
[ "$vflg" -a -t ] && cat $TMPDIR/$prog$$.lst

awk '
BEGIN {
	total = 0
	httpd = 0; forms = 0; java = 0; fnd = 0; sql = 0; other = 0
	httpd_tot = 0; forms_tot = 0; java_tot = 0; fnd_tot = 0; sql_tot = 0; other_tot = 0
	printf("%-8s %4s    %-8s %6s %6s %6s %6s %6s %6s\n", "SERVER", "ALL", "CLIENT",
		"HTTPD", "FORMS", "JAVA", "FND", "SQL", "OTHER")
}
$2 == "PROCESS" {
	next
}
/^$/ {
	if ($4 != client && length(client)) {
		printf(" %6d %6d %6d %6d %6d %6d",
			httpd, forms, java, fnd, sql, other)
		httpd = 0; forms = 0; java  = 0; fnd  = 0; sql  = 0; other  = 0
		printf("\n")
	}
	next
}
$3 == "<-" { 
	sub("\[", "", $2);
	sub("]", "", $2);
 
	if ($1 != server && length(server)) {
		printf("%8s %4d             %6d %6d %6d %6d %6d %6d\n\n",
			"", total, httpd_tot, forms_tot, java_tot, fnd_tot, sql_tot, other_tot)
		total = $2
		httpd_tot = 0; forms_tot = 0; java_tot = 0; fnd_tot = 0; sql_tot = 0; other_tot = 0
	} else
		total += $2
 
	printf("%s %4d <- %s", $1, $2, $4)
 
	server = $1
	client = $4
	next
}
$4 ~ /httpd/ {
	httpd++
	httpd_tot++
	next
}
$4 ~ /f60webmx/ {
	forms++
	forms_tot++
	next
}
$4 ~ /java/ {
	java++
	java_tot++
	next
}
$4 ~ /FNDCRM|FNDLIBR|FNDSCH|FNDFS|POXCON|RCVOLTM|INVLIBR|INCTM/ {
	fnd++
	fnd_tot++
	next
}
$4 ~ /sqlplus/ {
	sql++
	sql_tot++
	next
}
{
	other++
	other_tot++
}
END {
	if (httpd||forms||java||fnd||sql||other)
		printf(" %6d %6d %6d %6d %6d %6d\n",
			httpd, forms, java, fnd, sql, other)
	printf("%8s %4d             %6d %6d %6d %6d %6d %6d\n",
		"", total, httpd_tot, forms_tot, java_tot, fnd_tot, sql_tot, other_tot)
}' $TMPDIR/$prog$$.lst > $TMPDIR/$prog$$.awk
 
[ $fflg ] && cat $TMPDIR/$prog$$.awk >> $file
[ -t ] && cat $TMPDIR/$prog$$.awk 

rm -f $TMPDIR/$prog$$.sh $TMPDIR/$prog$$.lst $TMPDIR/$prog$$.out $TMPDIR/$prog$$.awk
