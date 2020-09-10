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
 * Oracle block information tool
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#ifdef __SVR4
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define	USAGE  "[-b blocksize] [-v] [file...]"
#define	LIBDIR "rdbms/lib"
#define	EXE    "bbed"
#define BINDIR "/opt/oracle/bin"
#define	MAKELINE BINDIR "/lnbbed; make -f ins_rdbms.mk $ORACLE_HOME/" LIBDIR "/" EXE
#define	COMMAND "./" EXE
#define	BS1K   1024
#define	BS2K   2048
#define	BS4K   4096
#define	BS8K   8192

#define	OFFCTL9  0x2020 // 020040 offset to find SID in ctl file 9i and 10g
#define	OFFCTL11 0x4020 // 040040 offset to find SID in ctl file 11g

#define	C_DBN  "print kcvfh.kcvfhhdr.kccfhdbn\n"
#define C_BLK  "print /d kcvfh.kcvfhbfh.type_kcbh\n"
//#define	C_BL2  "print /d kcbh.type_kcbh block 2\n"
#define	C_BL2  "print /d block 2\n"
#define	C_TSP  "print kcvfh.kcvfhhdr.kcvfhtnm\n"
#define	C_FSZ  "print /d kcvfh.kcvfhhdr.kccfhfsz\n"
#define	C_SCN  "print /d kcvfh.kcvfhckp.kcvcpscn.kscnbas\n"
#define	C_SC2  "print /d kcvfh.kcvfhrls.kscnbas\n"
#define	C_LOG  "dump /v offset 28 count 8\nfind /c Thread TOP\nfind /c Seq# TOP\n"

#define	BSDFL  BS2K
static int	bs = BSDFL;
static char bbedout[5*BUFSIZ];
static char	*oracle_home;

char *prog;

char *blocktypes[] = {
	/* 00 */ "reserved",
	/* 01 */ "undo segment header",
	/* 02 */ "undo data block",
	/* 03 */ "save undo header",
	/* 04 */ "save undo data block",
	/* 05 */ "data segment header",
	/* 06 */ "KTB managed data block",
	/* 07 */ "temp table data block",
	/* 08 */ "sort key",
	/* 09 */ "sort run",
	/* 10 */ "segment free list block",
	/* 11 */ "data file header"
	};

static int hidden[10] = {
	'B',
	'L',
	'O',
	'C',
	'K',
	'E',
	'D',
	'I',
	'T',
	'\0'
};
static char password[10];

#ifdef __STDC__
static char *bbed(char *, int, char *, char *);
static char *parse(char *, char *);
static int print_error(int);
static char *showpass(void);
static char *parse_log(char *);
static int parse_ctl(char *);
static char *get_sid(char *, long int);
#else
static char *bbed();
static char *parse();
static int print_error();
static char *showpass();
static char *parse_log();
static int parse_ctl();
static char *get_sid();
#endif

#if defined(__linux__) || defined(__sun)
#ifdef __STDC__
extern char *ltoa(long num);
#else
extern char *ltoa();
#endif
#endif

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	extern	char *optarg;
	extern	int optind;
	int		bflg = 0, vflg = 0, rflg = 0, errflg = 0;
	int		c;
	int		rel;
	int		errcount = 0;
	char	*out, *log;

	prog = strdup(basename(argv[0]));

	while ((c = getopt(argc, argv, "lb:v")) != -1)
		switch (c) {
		case 'v':		/* print full path */
			vflg++;
			break;
		case 'b':
			if ((bs = atoi(optarg)) == 0)
					errflg++;
			if (bs % BS1K) {
				fprintf(stderr, "%s: blocksize must be multiple of 1K\n", prog);
				errflg++;
			}
			break;
		case '?':
			errflg++;
		}
	
	if (argc - optind == 0)		/* need at least one file */
		errflg++;

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}

	if ((oracle_home = getenv("ORACLE_HOME")) == NULL) {
		fprintf(stderr, "%s: ORACLE_HOME must be set\n", prog);
		exit(1);
	}
	if (chdir(oracle_home) != 0) {
		fprintf(stderr, "%s: cannot chdir to %s\n", prog, oracle_home);
		exit(1);
	}

	if (chdir(LIBDIR) != 0) {
		fprintf(stderr, "%s: cannot chdir to %s/%s\n", prog, oracle_home,
			LIBDIR);
		exit(1);
	}
	if (access(EXE, X_OK) != 0) {
		/* BBED not built */
		if (system(MAKELINE) != 0) {
			fprintf(stderr, "%s: make command failed\n", prog);
			exit(1);
		}
		printf("%s: bbed executable built. Rerun %s\n", prog, prog);
		exit(2);
	}
	log = tempnam(NULL, EXE);

	while (optind < argc) {

		if (access(argv[optind], R_OK) != 0) {
			fprintf(stderr, "%s: cannot access %s\n", prog, argv[optind]);
			errcount++;	/* exit with error for access errors */
			goto loop;
		}
		if (vflg)
			printf("%s: %s", prog, argv[optind]);
		else
			printf("%s: %s", prog, basename(argv[optind]));

		/* Check for redo log */
		if ((out = parse_log(bbed(argv[optind], 1024, C_LOG, log))) != NULL) {
			printf(" redo log [%s]\n", out);
			goto loop;
		}

		/* Check for control file - type 21 */
		if (parse_ctl(bbed(argv[optind], bs, C_BLK, log))) {
			char *sid = get_sid(argv[optind], OFFCTL11);

			if (!strlen(sid))	/* try 9i control file format */
				sid = get_sid(argv[optind], OFFCTL9);

			printf(" control file [%s]\n", sid);
			goto loop;
		}

		/* Get the header block header */
		if ((out = parse("type_kcbh", bbed(argv[optind], bs, C_BLK, log))) == NULL) {
			errcount++;
			goto loop;	/* failed - print error and continue */
		}

		if (strcmp(out, "11") == 0) {
			printf(" datafile");
			/*	- Block 2 might not exist!!!
			if ((out = parse("type_kcbh", bbed(argv[optind], bs, C_BL2, log))) != NULL)
				printf(" %s", blocktypes[atoi(out)]);
				*/
		} else
			printf(" %s", blocktypes[atoi(out)]);

		/* Get the database name */
		if ((out = parse("kccfhdbn", bbed(argv[optind], bs, C_DBN, log))) == NULL) {
			errcount++;
			goto loop;	/* failed - print error and continue */
		}
		printf(" [%s]", out);

		/* Get the tablespace name */
		if ((out = parse("kcvfhtnm", bbed(argv[optind], bs, C_TSP, log))) == NULL) {
			errcount++;
			printf("\n");
			goto loop;	/* failed - print error and continue */
		}
		printf(" \"%s\"", out);

		/* Get the size */
		if ((out = parse("kccfhfsz", bbed(argv[optind], bs, C_FSZ, log))) == NULL) {
			errcount++;
			goto loop;	/* failed - print error and continue */
		}
		printf(" %s blks", out);

		/* Get the SCN */
		//if ((out = parse("kscnbas", bbed(argv[optind], bs, C_SC2, log))) == NULL)
			//exit(1);
		//printf("  kcvfhrls scn %s ", out);

		/* Get the SCN */
		//if ((out = parse("kscnbas", bbed(argv[optind], bs, C_SCN, log))) == NULL) {
		//	errcount++;
		//	goto loop;	/* failed - print error and continue */
		//}
		//printf(" c#%s", out);

		printf("\n");

	loop:	
		//printf("\n");
		optind++;
	}
	if (errcount)
		exit(1);

	exit(0);
}

static char *
#ifdef __STDC__
bbed(char *filename, int blocksize, char *cmds, char *log)
#else
bbed(filename, blocksize, cmds, log)
 char *filename;
 int  blocksize;
 char *cmds;
 char *log;
#endif
{
	char	ibuf[BUFSIZ];
	char	buf[BUFSIZ];
	int		opfd[2];	/* will write on this one */
	int		ipfd[2];	/* will read on this one */
	int		n;
	int		wstat;


	if (access(filename, R_OK) != 0) {
		fprintf(stderr, "%s: cannot access %s\n", prog, filename);
		return NULL;
	}

	if (pipe(opfd) == -1) {		/* set up pipelines */
		perror("opipe");
		exit(1);
	}
	if (pipe(ipfd) == -1) {
		perror("ipipe");
		exit(1);
	}

	switch (fork()) {
	case -1:
		perror("fork");
		exit(1);
	case 0:	/* child */
		if (close(0) == -1) {		/* make fd 0 (stdin) available */
			perror("close0c");
			exit(1);
		}
		if (dup(opfd[0]) != 0) {	/* make stdin output pipe */
			perror("odup0c");
			exit(1);
		}
		if (close(opfd[0]) == -1 || close(opfd[1]) == -1 ) {	/* don't need */
			perror("close2c");
			exit(1);
		}
		if (close(1) == -1) {		/* make fd 1 (stdout) available */
			perror("close1c");
			exit(1);
		}
		if (dup(ipfd[1]) != 1) {	/* make stdout input pipe */
			perror("idup1c");
			exit(1);
		}
		if (close(ipfd[0]) == -1 || close(ipfd[1]) == -1 ) {	/* don't need */
			perror("close2c");
			exit(1);
		}
		execl(EXE, EXE,
			"password=", showpass(), "mode=browse", "silent=y", "logfile=",
			log, (char *)NULL);
		perror("exec");
		exit(1);
	}
	/* parent */

	snprintf(ibuf, BUFSIZ, "set blocksize %d\n", blocksize);
	write(opfd[1], ibuf, strlen(ibuf));

	snprintf(ibuf, BUFSIZ, "set filename '%s'\n", filename);
	write(opfd[1], ibuf, strlen(ibuf));

	snprintf(ibuf, BUFSIZ, cmds);
	write(opfd[1], ibuf, strlen(ibuf));

	if (close(opfd[0]) == -1 || close(opfd[1]) == -1 ) { /* no longer needed */
		perror("close2");
		exit(1);
	}
	if (close(0) == -1) {		/* make fd 0 (stdin) available */
		perror("close0");
		exit(1);
	}
	if (dup(ipfd[0]) != 0) {	/* make stdin input pipe */
		perror("idup0");
		exit(1);
	}
	if (close(ipfd[0]) == -1 || close(ipfd[1]) == -1 ) { /* no longer needed */
		perror("close2");
		exit(1);
	}

	bbedout[0] = '\0';

	while (fgets(buf, BUFSIZ, stdin) != NULL) {
		strcat(bbedout, buf);
	}

	wait(&wstat);

	if (!WIFEXITED(wstat))
		return NULL;

	return bbedout;
}

/*
 * parse(): parses output from the PRINT command, looking for token "token"
 * in "in" and returning its value.
 */
static char	str[512];

static char *
#ifdef __STDC__
parse(char *token, char *in)
#else
parse(token, in)
 char *token;
 char *in;
#endif
{
	int		index;
	char	type[15];
	char	name[15];
	char	offset[15];
	char	ch;
	int		d;
	int		n;
	char	*s;

	if (!in) {
		print_error(400);
		return NULL;
	}
	str[0] = '\0';
	
	while ((s = strtok(in, "\n")) != NULL) {

		if (strstr(s, "BBED>"))	
			s = s + 6;

		if ((n = sscanf(s, "%s %[a-z][%3d] @%[0-9] %c\n",	/* array value */
				type, name, &index, offset, &ch)) == 5) {
			if (strstr(type, "BBED-") != NULL) {
				print_error(atoi(strchr(type, '-') + 1));
				return NULL;
			}
			if (strcmp(name, token) == 0) {		/* matches? */
					str[index] = ch;
					str[index+1] = '\0';
			}
		} else {
				n = sscanf(s, "%s %s @%[0-9] %d\n", type, name, offset, &d);
				if (strstr(type, "BBED-") != NULL) {
					print_error(atoi(strchr(type, '-') + 1));
					return NULL;
				}
				if (n == 4 && strcmp(name, token) == 0) {		/* matches? */
					strcpy(str, ltoa(d));
				}
		}

		in = NULL;
	}
	if (strlen(str))
		return str;

	return NULL;
}
/*
 * text kccfhdbn[1]                            @33      R
 * text kccfhdbn[2]                            @34      N
 * text kccfhdbn[3]                            @35      1
 * text kccfhdbn[4]                            @36
 * text kccfhdbn[5]                            @37
 * text kccfhdbn[6]                            @38
 * text kccfhdbn[7]                            @39
 *
 * BBED> ub1 type_kcbh                               @0        11
 */
static int
#ifdef __STDC__
print_error(int err)
#else
print_error(err)
 int err;
#endif
{
	char	buf[BUFSIZ];
	FILE	*fp;
	int		errnum, errtype;
	char	mesg[512];
	int		n, found = 0;

	snprintf(buf, BUFSIZ, "%s/rdbms/mesg/bbedus.msg", oracle_home);

	if ((fp = fopen(buf, "r")) == NULL) {
		fprintf(stderr, "cannot locate error file\n");
		return 0;
	}

	while (fgets(buf, BUFSIZ, fp) != NULL) {
		if (n = sscanf(buf, "%d, %d, \"%[^\"]\"", &errnum, &errtype, mesg)) {
			if (errtype == 1 && errnum == err) {
				printf("\n%s: %04d %s\n", prog, errnum, mesg);
				found++;
			}
		} else if (n = sscanf(buf, "//%[^/]", mesg) && buf[0] == '\\') {
			if (found)
				printf("//%s", mesg);
		} else {
			if (found)
				return 1;
		}
	}

	return 0;	/* error not found */
}

static char *
#ifdef __STDC__
showpass(void)
#else
showpass()
#endif
{
	int		*np;

	password[0] = '\0';
	np = hidden;

	while (*np)
		sprintf(password, "%s%c", password, *(np++));
	//printf("password is %s \n", password);

	return password;
}
/*
 * parse_log(): parses output from the log:
 *
 *	BBED> dump /v offset 28 count 8
 *	 File: /u07/oradata/TRN1/log02b.dbf (0)
 *	 Block: 1       Offsets:   28 to   35  Dba:0x00000000
 *	-------------------------------------------------------
 *	 54524e31 00000000                   l TRN1....
 *
 *	 <16 bytes per line>
 *
 */
static char *
#ifdef __STDC__
parse_log(char *in)
#else
parse_log(in)
 char *in;
#endif
{
	char	buf[BUFSIZ];
	int		b1, b2;
	char	offset[10];
	static char	sid[9];
	int		n, found = 0;
	char	*s;

	while ((s = strtok(in, "\n")) != NULL) {

		if (n = sscanf(s, "%s\n", buf)) {		/* check for error */
			if (strstr(s, "BBED-") != NULL) {
				return NULL;
			}
		}

		if ((n = sscanf(s, "%x %x %s %[A-Za-z0-9_-]\n",	&b1, &b2, offset, sid)) == 4) {
			found++;
		} 

		in = NULL;
	}
	if (found)
		return sid;

	return NULL;
}

static int
#ifdef __STDC__
parse_ctl(char *in)
#else
parse_ctl(in)
 char *in;
#endif
{
	char	buf[BUFSIZ];
	int		n;
	char	*s;

	while ((s = strtok(in, "\n")) != NULL) {

		if (n = sscanf(s, "%s\n", buf)) {		/* check for error */
			if (strstr(s, "BBED-00400: invalid blocktype (21)") != NULL) {
				return 1;
			}
		}

		in = NULL;
	}

	return 0;
}


/*
 * get_sid(): return the sid found at offset "offset" in the file "path"
 */
#define MAXSID	9

static char *
#ifdef __STDC__
get_sid(char *path, long int offset)
#else
get_sid(path, offset)
 char *path;
 long int offset;
 #endif
{
	char	buf[BUFSIZ];
	FILE	*fp;
	static char	sid[MAXSID];
	int		n = 0, c;

	strcpy(sid, "");

	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "cannot open %s\n", path);
		return NULL;
	}

	while ((c = fgetc(fp)) != EOF && n++ < offset-1)
		;
	while ((c = fgetc(fp)) != EOF && c != '\0')
		snprintf(sid, MAXSID, "%s%c", sid, c);

	fclose(fp);

	return sid;	/* error not found */
}
