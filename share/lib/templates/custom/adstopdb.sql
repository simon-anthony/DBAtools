REM $Header$
REM +========================================================================+
REM |  Copyright (c) 1997 Oracle Corporation Redwood Shores, California, USA
REM |                          All Rights Reserved
REM +========================================================================+
REM | FILENAME
REM |   adstopdb.sql
REM |
REM | DESCRIPTION
REM |   Script to shutdown database for %s_dbSid%
REM |
REM | USAGE
REM |   sqlplus /nolog adstopdb.sql <SHUTDOWN MODE>
REM |
REM | NOTES
REM |   $Revision$
REM | HISTORY
REM ===========================================================================
REM dbdrv: none

define USER="&1"
define MODE="&2"

REM internal/manager@%s_dbSid%;
REM / as sysdba
WHENEVER SQLERROR EXIT 1;
connect &USER;
shutdown &MODE

exit
