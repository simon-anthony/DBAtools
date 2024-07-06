#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif

/*
 * getcmd - get sqlplus/sqldba/svrmgrl command compatibility
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#ifdef linux
extern size_t bufsplit(char *, size_t, char **);
#endif

#define ITOA(i) i+48
#define SPACE	32		/* ASCII space    (decimal) */
#define ASTRSK	42		/* ASCII asterisk (decimal) */

#define	CONFIG	"/etc/opt/oracle/dbacmds"

static char	cmd[BUFSIZ];

char *
#ifdef __STDC__
getcmd(const char *s)
#else
getcmd(s)
 char *s;
#endif
{
	FILE 	*fp;
	char	buf[BUFSIZ];
	char	str[BUFSIZ];
	int		rel = -2, lev = -2, bra = -2, seq = -2;
	char	*p = NULL;
	int 	i, n = 0;
	char	*a[5];
	char	*s2 = strdup(s);

	bufsplit(":., \t", 0, (char **)0);

#ifdef DEBUG
	fprintf(stderr, "getcmd: string (%s)\n", s2);
#endif

	if ((n = bufsplit(s2, 5, a)) < 1) {
		fprintf(stderr, "bad version format (%d)\n", n);
		exit(1);
	}
	for (i = 0; i < n; i++) {
		switch(i) {
		case 0:
			rel = atoi(a[i]);
			break;
		case 1:
			lev = atoi(a[i]);
			break;
		case 2:
			bra = atoi(a[i]);
			break;
		case 3:
			seq = atoi(a[i]);
			break;
		}
	}	
	if ((fp = fopen(CONFIG, "r")) == NULL) {
#ifdef DEBUG
		fprintf(stderr, "getcmd: cannot open %s\n", CONFIG);
#endif
		return NULL;
	}

	n = 0;	/* use to count lines */

	while (fgets((char *)buf, BUFSIZ, fp) != NULL) {
		int		R = ASTRSK, L = ASTRSK, B = ASTRSK;

		n++;
		if (buf[0] == '#' )
            continue;

		if (sscanf((char *)buf, "%i.%i.%i %s", &R, &L, &B, str) != 4)
			if (sscanf((char *)buf, "%i.%i %s", &R, &L, str) != 3)
				if (sscanf((char *)buf, "%i %s", &R, str) != 2) {
				fprintf(stderr, "Bad record: %s\n", (char *)buf);
				continue;
			}

#ifdef DEBUG
		fprintf(stderr, "getcmd: [%2d] R = %2i, L = %2c, B = %2c (%s)", n,
			 R,
			(L == ASTRSK) ? L : ITOA(L),
			(B == ASTRSK) ? B : ITOA(B), str);
#endif

		if (rel == R) {
			if (lev == L) {
				if (bra == B) {
					strncpy(cmd, str, BUFSIZ); /* exact match */
#ifdef DEBUG
					fprintf(stderr, " * exact\n");
#endif
					p = cmd;
					break;
				} else if (B == ASTRSK) {
					strncpy(cmd, str, BUFSIZ); /* default */
#ifdef DEBUG
					fprintf(stderr, " * default branch");
#endif
					p = cmd;
				}
			} else if (L == ASTRSK) {
				strncpy(cmd, str, BUFSIZ); /* default */ 
#ifdef DEBUG
				fprintf(stderr, " * default level");
#endif
				p = cmd;
			}
		}

#ifdef DEBUG
		fprintf(stderr, "\ngetcmd:      r = %2i, l = %2c, b = %2c cmd=%s\n\n",
			rel,
			(lev < 0) ? SPACE : ITOA(lev),
			(bra < 0) ? SPACE : ITOA(bra),
				(p ? p : "NULL"));
#endif
	}

#ifdef DEBUG
	fprintf(stderr, "getcmd:===>  r = %2i, l = %2c, b = %2c cmd=%s\n",
		rel,
		(lev < 0) ? ASTRSK : ITOA(lev),
		(bra < 0) ? ASTRSK : ITOA(bra),
		(p ? p : "NULL"));
#endif

	fclose(fp);
	free(s2);

	return p;
}
