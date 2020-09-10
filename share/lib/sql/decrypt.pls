CREATE OR REPLACE
FUNCTION apps.decrypt(key IN VARCHAR2, value IN VARCHAR2) RETURN VARCHAR2
IS
/* $Header$ */
LANGUAGE JAVA
	NAME 'oracle.apps.fnd.security.WebSessionManagerProc.decrypt(java.lang.String, java.lang.String) return java.lang.String';
/
