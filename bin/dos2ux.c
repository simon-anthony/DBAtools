#ifdef __SUN__
#pragma ident "$Header"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$"
#endif

/* vim:syntax=c:sw=4:ts=4:
 *******************************************************************************
 * sftp: Wrapper to save output for Delivery Manager, and perform optional
 * ASCII conversion on retrieved files.
 *
 *******************************************************************************
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
#include <libgen.h>
#ifdef __linux
#include <linux/limits.h>
#endif
#include <limits.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>

#define BINARY	0
#define IMAGE	BINARY
#define ASCII	1
#define DOS2UX	1
#define UX2DOS	2

#define SUCCESS	0
#define FAILURE	2

#define dos2ux(path)	convert(path, DOS2UX)
#define ux2dos(path)	convert(path, UX2DOS)

#define USAGE "file..."

static char *prog;

static int convert(char *, int);
static int is_cntrl(char *);

int
main(int argc, char *argv[])
{
	int		c;
	char	*options, *value;
	extern char *optarg;
	extern int optind, opterr, optopt;
	int		errflg = 0;
	char	*out = NULL;
	char	*err = NULL;
	int     fmt, n = SUCCESS;

	opterr = 0; /* I'll do my own error messages, thanks */

	while ((c = getopt(argc, argv, "")) != -1)
		;

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}

	prog = basename(argv[0]);

	if (strcmp(prog, "dos2ux") == 0)
		fmt = DOS2UX;
	else
		fmt = UX2DOS;

	if (argc-optind == 0) {
		(void)convert("-", fmt);
		goto done;
	}

	while (argv[optind])
		if (convert(argv[optind++], fmt) < 0);
			n = FAILURE;
done:
	exit(n);
}

static int
convert(char * pathname, int format)
{
	FILE	*fp;
	char	buf[BUFSIZ];
	size_t	len;
	int		n = 0, lineno = 0, ncntrl = 0;

	if (strcmp(pathname, "-") == 0)
		fp = stdin;
	else {	
		if ((fp = fopen(pathname, "r")) == NULL) {
			fprintf(stderr, "%s: cannot open %s for reading\n", prog, pathname);
			return -1;
		}
	}

	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		len = strlen((char *)buf);
		lineno++;
		
		if (format == UX2DOS) {
			if (len >= 2 && buf[len-1] == '\n' && buf[len-2] != '\r') {
				buf[len-1] = '\r';
				n++;
			}
		} else if (format == DOS2UX) {
			if (len >= 2 && buf[len-1] == '\n' && buf[len-2] == '\r') {
				buf[len-1] = '\0';
				buf[len-2] = '\n';
				n++;
			}
		}

		if (is_cntrl((char *)buf))
			ncntrl++;

		if (fputs((char *)buf, stdout) == EOF) {
			perror("fputs");
			fprintf(stderr, "%s: cannot write to stdout\n", prog);
			return -1;
		}

		if (format == UX2DOS)
			if (buf[len-1] == '\r')
				(void) putc('\n', stdout);
	}
		
	fclose(fp);
	fflush(stdout);

	return n;
}

static int
is_cntrl(char *s)
{
	while (*s) {
		if (iscntrl(*s) && !(isblank(*s) || *s == '\n' || *s == '\r')) 
			return 1;
		s++;
	}
	return 0;
}
