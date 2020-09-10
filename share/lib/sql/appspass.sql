-- $Header$
-- run as sysdba
CREATE OR REPLACE 
FUNCTION apps.decrypt( key IN VARCHAR2, value IN VARCHAR2) RETURN VARCHAR2
AS
LANGUAGE JAVA
	NAME 'oracle.apps.fnd.security.WebSessionManagerProc.decrypt(java.lang.String, java.lang.String) return java.lang.String';
/
show err

CREATE OR REPLACE
FUNCTION apps.get_pwd RETURN VARCHAR2
AS
	ret VARCHAR2(100);
BEGIN
	SELECT (SELECT decrypt(fnd_web_sec.get_guest_username_pwd,
				   fnd_user.encrypted_foundation_password)
			FROM dual)
	INTO ret
	FROM fnd_user
	WHERE fnd_user.user_name = (
		SELECT SUBSTR(fnd_web_sec.get_guest_username_pwd, 1,
					  INSTR(fnd_web_sec.get_guest_username_pwd, '/') -1)
		FROM dual);
	RETURN ret;
END;
/
show err
