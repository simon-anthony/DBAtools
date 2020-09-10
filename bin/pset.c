#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif
/* vim:syntax=c:sw=4:ts=4:
********************************************************************************
* pset: return string suitable for PS1 prompt etc.
*
********************************************************************************
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <curses.h>
#include <term.h>
#include <string.h>
#include <pwd.h>
#ifdef _AIX
#include <sys/types.h>
#include <limits.h>
#endif

// ANSI graphics modes
struct pair {
	int 	value;
	char	*name;
};

#define COLOR	0
static struct pair colours[] = {
#define RED		31
	{ RED,		"red" },
#define GREEN	32
	{ GREEN,	"green" },
#define YELLOW	33
	{ YELLOW,	"yellow" },
#define BLUE	34
	{ BLUE,		"blue" },
#define MAGENTA	35
	{ MAGENTA,	"magenta" },
#define CYAN	36
	{ CYAN,		"cyan" },
#define WHITE	37
	{ WHITE,	"white" },
#define BLACK	30
	{ BLACK,	"black" },
	0
};

#define ATTR	1
static struct pair attrs[] = {
#define UL		4
	{ UL,		"ul" },
	{ UL,		"underline" },
#define BOLD	1
	{ BOLD,		"bold" },
#define OFF		0
	{ OFF,		"off" },
};
#define FG		0	// i.e. 3x
#define BG		10	// i.e. 4x

static void sg(int, int, int);
static int getitem(int, char *);
static char *username(int);

int
main(int argc, char *argv[]) {
	int		hflg=0, sflg=0, tflg=0, Tflg=0, rflg=0, lflg=0, wflg=0, errflg=0;
	char	*var = "$ORACLE_SID";
	char	*colourstr = NULL;	// format <foregrnd>[:<backgrnd>[:<attribs>]]
	char	*hostname;
	char	*sgr;
	int		c;
	int		fg = GREEN, bg = OFF, at = OFF;

	while ((c = getopt(argc, argv, "stlThrw")) != EOF)
		switch (c) {
		case 't':
			if (sflg) errflg++;
			tflg++; 
			var = "$TWO_TASK";
			break;
		case 'u':
			lflg++;
			break;
		case 'h':
			hflg++;
			break;
		case 's':
			if (tflg) errflg++;
			sflg++;
			break;
		case 'T':
			Tflg++;
			break;
		case 'r':
			rflg++;
			sgr = tigetstr("sgr0");
			break;
		case 'w':
			wflg++;
			break;
		case '?':
			errflg++;		
		}

	if (errflg) {
		fputs("usage: pset [-s|-t] [-u][-h][-T]\n", stderr);
		fputs("  -s  show the ORACLE_SID value in the colour SG_ORACLE_SID\n", stderr);
		fputs("  -t  show the TWO_TASK value in the colour SG_TWO_TASK\n", stderr);
		fputs("  -h  show the hostname in the string\n", stderr);
		fputs("  -T  add the current working directory to the window title bar\n", stderr);
		exit(2);
	}

	if (sflg && wflg) {	// if -s but TWO_TASK is set then override it
		if (getenv("TWO_TASK")) {
			var = "${TWO_TASK:-$ORACLE_SID}";
			tflg++;
		}
	}

	if (tflg) {
		if ((colourstr = getenv("SG_TWO_TASK")) == NULL)
			fg = RED;
	} else if (sflg) {
		 if ((colourstr = getenv("SG_ORACLE_SID")) == NULL)
		 	fg = GREEN;
	}

	if (colourstr) {
		if (fg = getitem(COLOR, strtok(colourstr, ":"))) 
			if (bg = getitem(COLOR, strtok(NULL, ":")))
				at = getitem(ATTR, strtok(NULL, ":"));
	}

	if (hflg) {
		int maxhostnamelen = sysconf(_SC_HOST_NAME_MAX);
		char *p;
		if ((hostname = (char *)malloc(maxhostnamelen)) == NULL) {
			perror("malloc");
			exit(1);
		}
		if (gethostname((char *)hostname, maxhostnamelen) < 0) {
			fprintf(stderr, "failed to get hostname\n");
			exit(1);
		}
		if ((p = strchr((char *)hostname, '.')) != NULL) *p = '\0';
	}

	if (Tflg) {
		// the ^A^M...^A sequence prevents non printables being used to
		// calculate the prompt width in ksh
		fputs("\x01\x0d\x1b]0;", stdout);
		
		fputs(username(lflg), stdout);

		if (hflg) {
			fputs("@", stdout);
			fputs(hostname, stdout);
		}

		fputs(":$PWD\x07\x01\x0d", stdout);
	}

	fputs(username(lflg), stdout);
	if (hflg) {
		fputs("@", stdout);
		fputs(hostname, stdout);
	}

	if (sflg || tflg) {
		fputs("[", stdout);
		sg(fg, bg, at);
		fputs(var, stdout);
		sg(OFF, OFF, OFF);
		fputs("]", stdout);
	}

	fputs(": ", stdout);

	if (rflg)
		fputs(sgr, stdout);

	exit(0);
}

static void
sg(int fg, int bg, int att) 
{
	printf("\x01");
	if (fg == 0) {
		printf("\x1b[%d", 0);
		goto out;
	} else 
		printf("\x1b[%d", FG + fg);

	if (bg)
		printf(";%d", BG + bg);
	if (att)
		printf(";%d", att);

out:
	printf("m");
	printf("\x01");
}

static int
getitem(int type, char *name)
{
	struct pair *p;

	if (type == COLOR)
		p = colours;
	else
		p = attrs;

	while (p->value) {
		if (strcasecmp(p->name, name) == 0)
			return p->value;
		p++;
	}

	return 0;
}

static char *
username(int uselogname)
{
	char	*s = NULL;
	struct passwd *p; 

	if (uselogname)
		if ((s = getlogin()) != NULL)
			return s;
	/* probably no controlling tty */
	if ((p = getpwuid(getuid())) == NULL) {
		perror("getpwuid");
		return NULL;
	}

	return p->pw_name;
}

