%{
#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid_y = "$Header$";
#endif
/* 
 * ctxvar.y:
 * 	get variables from AutoConfig context file
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
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#define CTX
#include "ctx.h"

/*#define YYDEBUG 0*/
#define YYMAXDEPTH 10000
#define SPECIAL	"><$&*'\";|\\~?{}[] \t#:"	/* shell special commands */

#define NONL	0
#define NL		1
#define PRINT(s,n) if (xflg) (n ? printf("%s\n",s) : printf("%s",s))

#define NOQUOTE	0
#define QUOTE	1
#define PRINTEQ(s,t,q) (q ? printf("%s='%s'\n",s,t) : printf("%s=%s\n",s,t))

static Variable **variables;
static char *variable = NULL;	/* The single one we are looking for */
static Variable **cur = NULL;	/* The current variable when using -e */

static int vflg = 0, pflg = 0, eflg = 0 , xflg = 0;

static char	*rval = NULL;

static int found = 0;

void 
ctxsetflags(int flags)
{
	if (CTX_ISPRINT(flags))
		xflg++;
	if (CTX_ISPRINTV(flags))
		vflg++;
	if (CTX_ISREGEX(flags))
		pflg++;
	if (CTX_ISEDIT(flags))
		eflg++;
}

void 
ctxset(int flg, Variable *s[])
{
	if (flg == CTX_VAR) {
		variables = s;
		if (s[0] != NULL) {
			variable = s[0]->name;
		}
		found = 0;
	}
}

char * 
ctxget(int flg)
{
	if (flg == CTX_CURVAL)
		return rval;
	return NULL;
}

int
ctxfound(void)
{
	return found;
}

extern int yyerror();
extern int yylex();
/*extern int yydebug = 1;*/

#ifdef YYBISON
#ifndef yylex
extern int yylex(void);
extern void yyerror(char *);
#endif
#endif

#ifdef __STDC__
static int match(const char *, char *);
static int match_r(char *, char *);
static int match_s(char *, char *);
static char *curval(void);
static int match_ss(char *s, Variable **);
#else
static int match();
static int match_r();
static int match_s()'
static char *curval();
static int match_ss();
#endif

%}

%union {	/* yylval */
	char *string;
	int number;
}

	/* literal keyword tokens */
%token <string>	OA_VAR
%token <string>	VARIABLE
%token <string>	STRING
%token <string>	VALUE

%%
seq_of_statements:
		statement
	|	statement seq_of_statements
	;

statement:
		OA_VAR '=' VARIABLE {
			if (variable) {
				if ((pflg && match_r($3, variable)) || match_s($3, variable)) {
					found++;
					if (vflg||pflg)
						PRINT($3, NL);
				}
			} else 
				PRINT($3, NL); }
	|	OA_VAR '=' VARIABLE VALUE { 
			if (variable) {
				if ((pflg && match_r($3, variable)) || (eflg && match_ss($3, variables)) || match_s($3, variable)) {
					found++;
					if (vflg||pflg) {
						if (strpbrk($4, SPECIAL) != NULL)
							PRINTEQ($3, $4, QUOTE);
						else
							PRINTEQ($3, $4, NOQUOTE);
					} else if (eflg) {
						PRINT(curval(), NONL);
					} else {
						PRINT($4, NL);
						rval = $4;
					}
				} else if (eflg)
					PRINT($4, NONL);
			} else {
				if (strpbrk($4, SPECIAL) != NULL)
					PRINTEQ($3, $4, QUOTE);
				else
					PRINTEQ($3, $4, NOQUOTE);
			}
		}
	;

%%
/*
 * match: match string s against regular expression pattern 
 */
#include <regex.h>
static int
#ifdef __STDC__
match(const char *s, char *pattern)
#else
match(s, pattern)
 char *s;
 char *pattern;
#endif
{
	int		status;
	static int compiled = 0;
	static regex_t	re;


	if (! compiled) {
		if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB|REG_ICASE) != 0) {
			fprintf(stderr, "ctxvar: invalid regular expression\n");
			return(0);
		}
		compiled = 1;
	}

	status = regexec(&re, s, (size_t)0, NULL, 0);

	if (status != 0)
		return(0);

	return(1);
}

/* match_r: match regex */
static int match_r(char *s, char *var)
{
	if (match(s, var))
		return 1;
	return 0;
}

/* match_s: match string */
static int match_s(char *s, char *var)
{
	if (strcmp(s, var) == 0)
		return 1;
	return 0;
}

/* curval: return value of current variable */
static char *curval(void)
{
	return (*cur)->value;
}

/* match_ss: match strings */
static int match_ss(char *s, Variable **vars)
{
	Variable **vptr;
	vptr = vars;
	while (*vptr) {
		if (match_s(s, (*vptr)->name)) {
			cur = vptr;
			return 1;
		}
		*(vptr)++;
	}
	return 0;
}
