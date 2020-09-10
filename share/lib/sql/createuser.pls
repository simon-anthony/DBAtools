CREATE OR REPLACE
PROCEDURE apps.xxcreateuser(user_name IN VARCHAR2, description IN VARCHAR2) 
AUTHID DEFINER
AS
/* $Header$ */
BEGIN
	apps.fnd_user_pkg.createuser(x_user_name => user_name, x_description => description, x_owner => '');
END;
/
