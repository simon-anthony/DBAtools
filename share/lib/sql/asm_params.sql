-- $Header$
col "Parameter" for a32
col "Description" for a32
col "Instance Value" for a32
SELECT a.ksppinm "Parameter", 
	a.ksppdesc "Description",
	c.ksppstvl "Instance Value"
FROM x$ksppi a, 
	x$ksppcv b,
	x$ksppsv c
WHERE a.indx = b.indx and a.indx = c.indx
  AND ksppinm like '%asm%'
ORDER BY a.ksppinm;
