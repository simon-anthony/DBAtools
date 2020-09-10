REM $Header$
REM +========================================================================+
REM |  Copyright (c) 1997 Oracle Corporation Redwood Shores, California, USA
REM |                          All Rights Reserved
REM +========================================================================+
REM | FILENAME
REM |   adsrttdb.sql
REM |
REM | DESCRIPTION
REM |   Script to startup database
REM |
REM | USAGE
REM |   sqlplus /nolog @adstrtdb.sql
REM |
REM | NOTES
REM |   $Revision$
REM |   Removed the pfile location :  %s_db_oh%/dbs/init%s_dbSid%.ora
REM |   Script to startup database with spfile if exists     
REM |
REM | HISTORY
REM =========================================================================+
REM
REM $AutoConfig$
REM
REM dbdrv: none

REM connect / as sysdba;
WHENEVER SQLERROR EXIT 9
define USER="&1"
define MODE="&2"
define ARG="&3"
connect &USER;

startup &MODE &ARG

exit

