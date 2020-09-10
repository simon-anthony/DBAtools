D		[0-9]
L		[?a-zA-Z]
W		[ \t]+
%{
#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid_l = "$Header$";
#endif
/*
 * ctxvar.l:
 *  get tokens for ctxvar.y from a context file
 *
 *  Permutations:
 *
 *   VARIABLE VALUE
 *     <service_names oa_var="s_dbService">ENZO</service_names>
 *   VARIABLE VALUE
 *     <lib_ext oa_var="s_lib_ext" osd="HP_UX">sl</lib_ext>
 *     <dbport oa_var="s_dbport" oa_type="PORT">1541</dbport>
 *   VARIABLE
 *     <quorum_disk oa_var="s_dbQuorumDisk"/>
 *   VARIABLE
 *     <appsuser oa_var="s_appsuser" osd="unix"/>
 *
 * Copyright (C) 2008 Simon Anthony
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License or, (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program. If not see <http://www.gnu.org/licenses/>>
 */ 
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PUT if (edit) fputs((char *)yytext, stdout)
#define PUTS(s) if (edit) fputs((char *)s, stdout)
#define PUTC(c) if (edit) fputc(c, stdout)

#undef  YYLMAX
#define YYLMAX 24576

static void skipcomments(void);
//static char *var;
static int edit = 0;

void
ctxsetedit(int mode)
{
	edit = mode;
}

%}

%a 4000
%o 5000

%s KYWD
%s VAR
%s VAL

%%
"<!"		skipcomments();
\n			{ PUT; /* strip newlines */ }
\r			{ PUT; /* strip returns */ }
{W}			{ PUT; /* strip whitespace */ }
"oa_var"	{
	BEGIN KYWD;
	PUT;
	return OA_VAR;
}
<KYWD>"=" {
	PUT;
	return yytext[0];
}
<KYWD>\" {
	PUT;
	BEGIN VAR;
}
<VAR>[^"\n<>=\ \t]+ {
	int	c2, c1;
	PUT;
	yylval.string = strdup((char *) yytext);

	for (;;) {	/* munch other assignments in this section */
		if ((c2 = input()) == '>') {
			unput(c2);
			break;
		}
		c1 = c2;
		PUTC(c2);
	}
	if (c1 == '/')
		BEGIN INITIAL;	/* no value */
	else {
		c2 = input();	/* munch the > */
		PUTC(c2);
		BEGIN VAL;
	}
	return VARIABLE;
}
<VAL>[^<]+/< {
	yylval.string = strdup((char *) yytext);
	return VALUE;
}
<VAL></\/ {
	PUT;
	BEGIN INITIAL;	/* munch*/
}
.			{ PUT; /* munch */ }
%%
static void
skipcomments(void) {
	int		c1 = 0, c2 = input();

	PUT;
	for (;;) {
		if (c2 == EOF)
			break;
		if (c1 == '-' && c2 == '>') {
			PUTC(c2);
			break;
		}
		c1 = c2;
		c2 = input();
		PUTC(c1);
	}
}

int
yyerror(char *msg)
{
	fprintf(stderr, "line %d:\n%s\n** %s\n", yylineno, yytext, msg);
	exit(5);
}

int
yywrap()
{
	return 1 ;
}
