CREATE OR REPLACE
FUNCTION apps.get_fnd_pwd(user fnd_user.user_name%TYPE DEFAULT 'APPS') RETURN VARCHAR2
IS
/* $Header$ */
	ret VARCHAR2(100);
BEGIN
	IF user = 'APPS'
	THEN
		SELECT (SELECT decrypt(fnd_web_sec.get_guest_username_pwd,
					   fnd_user.encrypted_foundation_password)
				FROM dual)
		INTO ret
		FROM fnd_user
		WHERE fnd_user.user_name = (
				SELECT SUBSTR(fnd_web_sec.get_guest_username_pwd, 1,
						  INSTR(fnd_web_sec.get_guest_username_pwd, '/') -1)
				FROM dual);
	ELSE
		SELECT decrypt(UPPER((
			SELECT(
				SELECT decrypt(UPPER(
					fnd_web_sec.get_guest_username_pwd),
					fnd_user.encrypted_foundation_password)
				FROM dual) AS apps_password
			FROM fnd_user
			WHERE fnd_user.user_name = UPPER((
				SELECT SUBSTR(fnd_web_sec.get_guest_username_pwd, 1,
					INSTR(fnd_web_sec.get_guest_username_pwd, '/') -1)
				FROM dual)))),
			fnd_user.encrypted_user_password)
		INTO ret
		FROM fnd_user
		WHERE fnd_user.user_name = UPPER(user);
	END IF;

	RETURN ret;
EXCEPTION
	WHEN NO_DATA_FOUND
	THEN
		RETURN NULL;
END;
/
