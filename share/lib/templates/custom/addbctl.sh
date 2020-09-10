#!/bin/sh
# $Header$
# *===========================================================================+
# |  Copyright (c) 1996 Oracle Corporation, Redwood Shores, California, USA   |
# |                        All rights reserved                                |
# |                       Applications  Division                              |
# +===========================================================================+
# |
# | FILENAME
# |   addbctl.sh
# |
# | DESCRIPTION
# |   Start / Stop database %s_dbSid%
# |
# | USAGE
# |   addbctl.sh [start|stop] {immediate|abort|normal}
# |
# | PLATFORM
# |   Unix Generic
# |
# | NOTES
# |   $Revision$
# | HISTORY
# | 
# +===========================================================================+
# dbdrv: none 

header_string="$Header$"
prog_version=`echo "$header_string" | awk '{print $3}'`
program=`basename $0`
usage="\t$program [start|stop] {normal|immediate|abort}"

printf "\nYou are running $program version $prog_version\n\n"

if [ $# -lt 1 ];
then
   printf "\n$program: too few arguments specified.\n\n"
   printf "\n$usage\n\n"
   exit 1;
fi

control_code="$1"

if test "$control_code" != "start" -a "$control_code" != "stop" ; then
   printf "\n$program: You must either specify 'start' or 'stop'\n\n"
   exit 1;
fi

shutdown_mode="normal"

DB_VERSION="%s_database%"

#
# We can't change "internal" to "/ as sysdba" for 817 - see bug 2683817.
#

if [ "$DB_VERSION" = "db817" ]
then
  priv_connect="internal"
else
  priv_connect="/ as sysdba"
fi 

if test "$control_code" = "stop"; then
  if test $# -gt 1; then
    shutdown_mode="$2";

    if test "$shutdown_mode" != "normal" -a \
            "$shutdown_mode" != "immediate" -a \
            "$shutdown_mode" != "abort" ; then
      printf "\n$program: invalid mode specified for shutdown\n"
      printf "\tThe mode must be one of 'normal', 'immediate', or 'abort'\n\n"
      exit 1;
    fi
  fi
fi

ORA_ENVFILE="%s_db_oh%/%s_contextname%.env"
DB_NAME="%s_dbSid%"

#
# setup the environment for Oracle and Applications
#

if [ ! -f $ORA_ENVFILE ]; then
   printf "Oracle environment file for database $DB_NAME is not found\n"
   exit 1;
else
   . $ORA_ENVFILE
fi

if test "$control_code" = "start" ; then
   printf "\nStarting the database $DB_NAME ...\n\n"
   sqlplus /nolog @%s_db_oh%/appsutil/scripts/%s_contextname%/adstrtdb.sql "$priv_connect" "$3" "$2"
   exit_code=$?

else

   printf "\nShutting down database $DB_NAME ...\n\n"
   sqlplus /nolog @%s_db_oh%/appsutil/scripts/%s_contextname%/adstopdb.sql "$priv_connect"  $shutdown_mode
   exit_code=$?

fi

printf "\n$program: exiting with status $exit_code\n\n"
exit $exit_code

