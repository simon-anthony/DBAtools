CREATE OR REPLACE
FUNCTION apps.get_pwd RETURN VARCHAR2
AS
/* $Header$ */
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
