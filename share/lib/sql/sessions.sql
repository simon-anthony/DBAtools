-- $Header$
set linesize 210 pagesize 50000

column sid				format 99999	heading	"SID"
column "Server OS User"	format a8		heading	"Server|OS User"
column spid				format a6		heading	"Server|PID"
column "Server Program"	format a28		heading	"Server|Program"
column "Client OS User"	format a8		heading	"CLient|OS User"
column username			format a10		heading	"Oracle|User"
column process			format a9		heading	"Client|PID"
column status			format a8		heading	"Status"
column machine			format a21		heading	"Client|Machine"
column client_program	format a32		heading	"Client|Program"
column sql_text			format a64		heading	"SQL"

clear breaks computes

break on sid on "Server OS User" on spid on "Server Program" on "Client OS User" on username on process on status on machine on client_program

SELECT
	s.sid,
	p.username		"Server OS User",
	p.spid,
	p.program		"Server Program",
	s.osuser		"Client OS User",
	s.username,
	s.process,
	s.status,
	DECODE (INSTR(machine, '.'),
		0, machine,
		   SUBSTR(s.machine, 1, INSTR(s.machine, '.') -1)) machine,
		NVL(s.program, 'none')	client_program,
	q.sql_text
FROM v$process p, v$session s, v$sqltext q
WHERE p.addr = s.paddr
AND q.address (+) = s.sql_address
AND q.hash_value (+) = s.sql_hash_value
AND s.type != 'BACKGROUND'
AND s.machine IS NOT NULL
AND s.sid != sys_context('USERENV', 'SID')
ORDER BY s.machine, s.process, s.program, s.status, q.piece
/
