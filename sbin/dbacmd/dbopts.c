#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#define _DBOPTS
#include "dbopts.h"

/*
 * dbopts: dbstart/dbshut option handling
 */

static int  prg;

void
#ifdef __STDC__
setprog(char *s)
#else
setprog(s)
 char *s;
#endif
{
	if (!strcmp(s, "dbstart") || !strcmp(s, "startup"))
		prg = DBSTART;
	else
		prg = DBSHUT;
}

/*
 * chkopt: check or set options.
 */
int
#ifdef __STDC__
chkopt(char *s, int n)
#else
chkopt(s, n)
 char *s;
 int n;
#endif
{
	struct	option *t;
	char	*s1;
	char	*asgmt = NULL;	/* is parameter assigned a value, e.g. pfile= */

	if (IS_DBSTART)
		t = startopts;
	else
		t = shutopts;

	while (t->name) {
		if (s) {
			s1 = strdup(s);
			asgmt = strtok(s1, "=");	/* mask any "=" sign */
			asgmt = strtok(NULL, "");	

			if (!strcmp(s1, t->name)) 
				if (n == SET) {
					t->flg++;
					if (asgmt) 
						t->value = strdup(asgmt);
					free(s1); 
					return 1;
				} else if (n == GET) {
					free(s1);
					return t->flg;
				}
			free(s1);

		} else if (n == PRINT) {
			if (t->flg)
				printf(t->value ? "%s=%s " : "%s ", t->name, t->value);
		}
		t++;
	}
	if (n == PRINT)
		printf("\n");
	return 0;
}

/*
 * valopt: validate options or modes
 */
int
#ifdef __STDC__
valopt(void)
#else
valopt()
#endif
{
	if (IS_DBSTART) {
		if ((chkopt("write", GET) || chkopt("only", GET)) &&
			!chkopt("read", GET))
			return 0;

		if (chkopt("only", GET) && chkopt("write", GET))
			return 0;

		if (chkopt("read", GET) && !chkopt("open", GET))
			return 0;

		if (chkopt("mount", GET) + chkopt("nomount", GET) +
			chkopt("open", GET) > 1)
			return 0;

	} else if (IS_DBSHUT) {
		if (chkopt("immediate", GET) + chkopt("abort", GET) + 
			chkopt("transactional", GET) +
			chkopt("normal", GET) > 1)
			return 0;

		if (chkopt("local", GET) && !chkopt("transactional", GET))
			return 0;
	}
	return 1;
}

/*
 * stropt: return options as a string, NULL if no options
 */
char *
#ifdef __STDC__
stropt(void)
#else
stropt()
#endif
{
    char    *s1 = NULL, *s2 = NULL;
	struct	option *t;

	if (IS_DBSTART)
		t = startopts;
	else
		t = shutopts;

	while (t->name) {
#ifdef DEBUG
		if (t->value)
			fprintf(stderr, "%-10s [%c] = %s\n",
				t->name, t->flg ? 'X' : ' ', t->value);
		else
			fprintf(stderr, "%-10s [%c]\n",
				t->name, t->flg ? 'X' : ' ');
#endif
		if (t->flg) {
			if ((s2 = (char *)malloc((s1 ? strlen(s1) : 0) +
						strlen(t->name) + 
						(t->value ? (strlen(t->value) + 1) : 0) + 2)) == NULL) {
				perror("malloc");
				exit(errno);
			}
			if (s1) {
				if (t->value)
					sprintf(s2, "%s %s=%s", s1, t->name, t->value);
				else
					sprintf(s2, "%s %s", s1, t->name);
				free(s1);
			} else {
				if (t->value)
					sprintf(s2, "%s=%s", t->name, t->value);
				else
					sprintf(s2, "%s", t->name);
			}
			s1 = s2;
		}
		t++;
	}

    return s1;
}
