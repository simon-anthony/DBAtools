CREATE OR REPLACE
PROCEDURE apps.xxaddresp(user_name IN VARCHAR2, resp_shortname IN VARCHAR2, resp_name IN VARCHAR2)
AUTHID DEFINER
AS
/* $Header$ */
	respkey		fnd_responsibility.responsibility_key%TYPE;
BEGIN
	SELECT responsibility_key
	INTO respkey
	FROM fnd_responsibility r, fnd_responsibility_tl rtl, fnd_application a
	WHERE a.application_short_name = resp_shortname
	AND a.application_id = r.application_id
	AND r.responsibility_id = rtl.responsibility_id
	AND rtl.responsibility_name = resp_name;

	apps.fnd_user_pkg.addresp(
		username => user_name,
		resp_app => resp_shortname,
		resp_key => respkey,
		security_group => 'STANDARD',
		description => '',
		start_date => SYSDATE,
		end_date => NULL);
END;
/
