--
-- Copyright (c) 1988, 2005, Oracle.  All Rights Reserved.
--
-- NAME
--   glogin.sql
--
-- DESCRIPTION
--   SQL*Plus global login "site profile" file
--
--   Add any SQL*Plus commands here that are to be executed when a
--   user starts SQL*Plus, or uses the SQL*Plus CONNECT command.
--
-- USAGE
--   This script is automatically run
--
set termout off

host columns=${COLUMNS:-`tput cols`}; lines=${LINES:-`tput lines`}; [ -t ] && { echo set linesize ${columns:-80} pagesize ${lines:-24}; if [ $TWO_TASK ]; then colour=${SG_TWO_TASK:-red}; else colour=${SG_ORACLE_SID:-green}; fi; case $colour in black) colour=0;; red) colour=1;; green) colour=2;; yellow) colour=3;; blue) colour=4;; magenta) colour=5;; cyan) colour=6;; white) colour=7;; *) colour=0 ; esac; echo set sqlprompt "\"[3${colour}m''_CONNECT_IDENTIFIER''[0m> \""; }  > $HOME/.glogin.sql

@$HOME/.glogin.sql
set termout on
