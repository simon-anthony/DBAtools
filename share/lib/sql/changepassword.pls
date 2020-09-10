CREATE OR REPLACE
FUNCTION apps.xxchangepassword(user_name IN VARCHAR2, password IN VARCHAR2) RETURN BOOLEAN
AUTHID DEFINER
AS
/* $Header$ */
BEGIN
	RETURN apps.fnd_user_pkg.changepassword(username => user_name, newpassword => password);
END;
/
