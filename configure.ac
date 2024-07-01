/* $Header$ */
AC_INIT([dbatools],[2.6],[bug@dbatools.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_AWK([gawk] nawk awk)
AC_MSG_CHECKING([for an ORACLE_HOME])
AC_DEFUN([SET_ORACLE_HOME],
	[ if oratab=`{ ls /var/opt/oracle/oratab || ls /etc/oratab; } 2>&-`; then
		oracle_home="`$AWK -F: '@S|@1 ~ /^@<:@A-Za-z@:>@/ { print @S|@2 }' @S|@oratab`";
		if [ test -x $oracle_home/bin/oracle ]; then
			AC_MSG_RESULT([$oracle_home]);
		else
			AC_MSG_RESULT([no]);
			AC_MSG_ERROR([no ORACLE_HOME found]);
		fi
	else
		AC_MSG_RESULT([no]);
		AC_MSG_ERROR([no ORACLE_HOME found]);
	fi ])
SET_ORACLE_HOME
AC_SUBST([ORACLE_HOME], $oracle_home)
AC_PROG_CC([xlc cc gcc cl])
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_RANLIB([ranlib])
AM_PROG_AR
LT_INIT([aix-soname=both])
AM_CONDITIONAL([LINUX], [test `uname -s` = Linux] )
AM_CONDITIONAL([SUN], [test `uname -s` = SunOS] )
AM_CONDITIONAL([HPUX], [test `uname -s` = HP-UX] )
AM_CONDITIONAL([AIX], [test `uname -s` = AIX] )
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
Makefile
bin/Makefile
bin/adsvcctl/Makefile
bin/ctxvar/Makefile
bin/ps/Makefile
etc/Makefile
etc/oracle/Makefile
include/Makefile
lib/Makefile
sbin/Makefile
sbin/cmmaintpkg.d/Makefile
sbin/dbacmd/Makefile
share/Makefile
share/lib/Makefile
share/lib/functions/Makefile
share/lib/rpt/Makefile
share/lib/rpt/ps/Makefile
share/lib/rpt/ps/aln/Makefile
share/lib/rpt/ps/apc/Makefile
share/lib/rpt/ps/cm/Makefile
share/lib/rpt/ps/db/Makefile
share/lib/rpt/ps/dln/Makefile
share/lib/rpt/ps/fmc/Makefile
share/lib/rpt/ps/fms/Makefile
share/lib/rpt/ps/frm/Makefile
share/lib/rpt/ps/opmn/Makefile
share/lib/rpt/ps/rep/Makefile
share/lib/rpt/tns/Makefile
share/lib/rpt/tns/aln/Makefile
share/lib/rpt/tns/dln/Makefile
share/lib/shell/Makefile
share/lib/sql/Makefile
share/man/Makefile
share/man/man1/Makefile
share/man/man3/Makefile
share/nlsdata/Makefile
])
AC_ARG_WITH([oracle_home],
	[AS_HELP_STRING([--with-oracle-home=[DIR]],
		[specify Location of ORACLE_HOME])],
	AS_IF([test -x "$withval"/bin/oracle],
		[AC_SUBST([ORACLE_HOME], $withval)],
		AC_MSG_FAILURE([invalid ORACLE_HOME specified])
	)
)
AS_IF([test -f $oracle_home/lib/libclntsh.so], 
	AC_MSG_NOTICE([libclntsh found]),
	AC_MSG_FAILURE([$oracle_home/bin/genclntsh not found])
)
AC_OUTPUT
