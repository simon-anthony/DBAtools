#ifndef _HPUX_SOURCE
#pragma ident "$Header$"
#else
static char *svnid = "$Header$";
#endif

/*
 * pps: quick ps(1)
	The POSIX spec has a strange distinction between SZ and vsz.
	Observe:

	SZ    The size in blocks of the core image of the process.

	vsz   The size of the process in (virtual) memory in
		  1024 byte units as a decimal integer.

		  For vsz, the term "memory" is used. For SZ, the term "core"
		  is used instead. If ancient terms were being used, I'd expect
		  to see "core" in both places. This seems to suggest that the
		  spec really is referring to what would be produced if the
		  process could and did produce a core dump.

    HP-UX observes this distinction. However, the manual page for ps(1)
	is incorrect in stating that the difference between SZ and VSZ is simply
	the units in which they are reported (pages and KBs respectively).
 *
 */

#define USAGE "[-x] -p proclist -o format"

#include <sys/param.h>
#include <sys/pstat.h>
#include <sys/dk.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <pwd.h>
#include <sched.h>
#include <ctype.h>

#define	min(a, b)	((a) > (b) ? (b) : (a))
#define	max(a, b)	((a) < (b) ? (b) : (a))

struct field {
	struct field	*next;		/* linked list */
    int				fname;		/* field index */
    char			*header;	/* header to use */
    int				width;		/* width of field */
};
    
static	struct field *fields = NULL;	/* fields selected via -o */
static	struct field *last_field = NULL;
static	int do_header = 0;
static	struct timeval now;

#ifdef __STDC__
static char *parse_format(char *);
static void print_field(struct pst_status *, struct field *);
static void print_time(time_t, int);
static void prtpct(float, int);
static void pr_fields(struct pst_status *, void (*print_fld)(struct pst_status *, struct field *));
#else
static char *parse_format();
static void print_field();
static void print_time();
static void prtpct();
static void pr_fields();
#endif

static char 	*prog;
static long 	sc_page_size;
static int		page_size;
//#define MAX_LENGTH 1024
static char	long_command[ARG_MAX];
static union pstun pu;

int extended = 0;	/* available externally */

static int pids[BUFSIZ];
static int npids = 0;

enum fname {
	F_ADDR,
	F_ARGS,
	F_CLASS,	/* F_CLS */
	F_COMM,
	F_C,		/* F_CPU */
	F_ETIME,
	F_F,		/* F_FLAGS */
	F_INTPRI,
	F_GID,
	F_GROUP,
	F_NICE,
	F_PCPU,
	F_PGID,
	F_PID,
	F_PPID,
	F_PRI,
	F_PRMID,
	F_PRMGRP,
	F_RGID,
	F_RGROUP,
	F_RUID,
	F_PSET,
	F_RUSER,
	F_SCHED,
	F_SID,
	F_S,		/* F_STATE */
	F_SZ,
	F_TIME,
	F_TTY,
	F_UID,
	F_USER,
	F_VSZ,
	F_WCHAN
};


/* array of defined fields, in fname order */
struct def_field {
	char 	*fname;
	char 	*header;
	int width;
	int minwidth;
};

static struct def_field fname[] = {
	/* fname	header		width	minwidth */
	{ "addr", 	"ADDR",		16,		 8 	},
	{ "args", 	"COMMAND",	80,		80	},
	{ "cls", 	"CLS",		 4,		 2	},
	{ "comm", 	"COMMAND",	15,		 8	},
	{ "cpu", 	"C",		 2,		 2	},	/* obsolete */
	{ "etime", 	"ELAPSED", 	11,		 7	},
	{ "flags", 	"F",		 2,		 2	},
	{ "intpri", "PRI",		 3,		 2	},	/* obsolete */
	{ "gid", 	"GID",		 5,		 5	},
	{ "group", 	"GROUP",	 8,		 8	},
	{ "nice", 	"NI",		 2,		 2	},
	{ "pcpu", 	"%CPU",		 4,		 4	},
	{ "pgid", 	"PGID",		 5,		 5	},
	{ "pid", 	"PID",		 5,		 5	},
	{ "ppid", 	"PPID",		 5,		 5	},
	{ "pri", 	"PRI",		 3,		 2	},
	{ "prmid", 	"PRMID",	12,		12	},
	{ "prmgrp", "PRMGRP",	12,		12	},
	{ "rgid", 	"RGID",		 5,		 5	},
	{ "rgroup", "RGROUP",	 8,		 8	},
	{ "ruid", 	"RUID",		 5,		 5	},
	{ "pset", 	"PSET",		 3,		 3	},
	{ "ruser", 	"RUSER",	 8,		 8	},
	{ "sched", 	"SCHED",	11,		 2	},
	{ "sid", 	"SID", 		 5,		 5	},
	{ "state", 	"S",		 1,		 1	},
	{ "sz", 	"SZ",		 6,		 6	},
	{ "time", 	"TIME",		 7,		 5	},
	{ "tty", 	"TTY",		 7,		 7	},
	{ "uid", 	"UID",		 5,		 5	},
	{ "user", 	"USER",		 8,		 8	},
	{ "vsz", 	"VSZ",		 6,		 6	},
	{ "wchan", 	"WCHAN",	16,		 8	}
};

#define	NFIELDS	(sizeof (fname) / sizeof (fname[0]))

static int oflg = 0, pflg = 0, xflg = 0, errflg = 0;


int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int	argc;
 char 	*argv[];
#endif
{
	char	*p;
	extern	char *optarg;
	extern	int optind, opterr;
	char	*options, *value;
	int		c;
	char	buf[BUFSIZ];
	char	*arg, *args;
	int	 pid;
	struct pst_static pss;
	struct pst_dynamic psd;
	struct pst_status *pst = NULL;
	struct pst_status *pstp = NULL;
	int		n, i, j, k;
	int		found = 0;


	prog = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "o:p:x")) != -1)
		switch (c) {
		case 'o':
			oflg++;
			p = optarg;
			while ((p = parse_format(p)) != NULL)
				;
			break;
		case 'p':		/* csv list of pids */
			pflg++;
			args = optarg; /* parse pids */

			while ((arg = strtok(args, ", ")) != NULL) {
				if (pid = atoi(arg)) 
					pids[npids++] = pid;
				args = NULL;
			}
			break;
		case 'x':
			xflg++;
			break;
		case '?':
			errflg++;
			break;
		}

	if (!(oflg && pflg))			/* -o and -p mandatory */
		errflg++;

	if (errflg) {
		fprintf(stderr, "%s: %s\n", prog, USAGE);
		exit(2);
	}

	if ((sc_page_size = sysconf(_SC_PAGE_SIZE)) == -1) {
		perror("sysconf");
		exit(1);
	}

	if (pstat_getstatic(&pss, sizeof(pss), (size_t)1, 0) == -1) {
		perror("pstat_getstatic");
		exit(1);
	}
	page_size = pss.page_size;

	if (xflg)
		pu.pst_command = long_command;

	if (pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1) {
		perror("pstat_getdynamic");
		exit(1);
	}

	if ((pst = (struct pst_status *)malloc(sizeof(struct pst_status) * psd.psd_maxprocs)) == NULL) {
		perror("malloc");
		exit(1);
	}

	if ((n = pstat_getproc(pst, sizeof(struct pst_status), psd.psd_activeprocs, 0)) == -1) {
		perror("pstat_getproc");
		exit(1);
	}

	if (fields) {	/* print user-specified header */
		if (do_header) {
			struct field *f;

			for (f = fields; f != NULL; f = f->next) {
				if (f != fields)
					(void) printf(" ");
				switch (f->fname) {
				case F_TTY:
				case F_SCHED:
					(void) printf("%-*s",
					    f->width, f->header);
					break;
				case F_COMM:
				case F_ARGS:
					/*
					 * Print these headers full width
					 * unless they appear at the end.
					 */
					if (f->next != NULL) {
						(void) printf("%-*s",
						    f->width, f->header);
					} else {
						(void) printf("%s",
						    f->header);
					}
					break;
				default:
					(void) printf("%*s",
					    f->width, f->header);
					break;
				}
			}
			(void) printf("\n");
		}
	}

	for  (i = 0; i < npids; i++) {
						
		pstp = pst;

		found = 0;

		for (j = 0; j < n; j++, pstp++) {

			if (pstp->pst_pid != pids[i])
				continue;

			found++;

			if (fields != NULL)
			   pr_fields(pstp, print_field);
		}

		if (!found) {
			int	width;
			if (fields != NULL) {
				struct field *f;

				for (f = fields; f != NULL; f = f->next)
					if (f->fname == F_PID) 
						break;
				width = f->width;
			} else {
				struct def_field *df;
			
				for (df = &fname[0]; df < &fname[NFIELDS]; df++)
					if (strcmp("pid", df->fname) == 0)
						break;
				width = df->width;
			}

			(void) printf("%*d <not found>\n", width, pids[i]);
		}
	}
}

/*
 * parse_format() takes the argument to the -o option,
 * sets up the next output field structure, and returns
 * a pointer to any further output field specifier(s).
 * As a side-effect, it increments errflg if encounters a format error.
 */
static char *
#ifdef __STDC__
parse_format(char *arg)
#else
parse_format(arg)
 char *arg;
#endif
{
	int c;
	char *name;
	char *header = NULL;
	int width = 0;
	struct def_field *df;
	struct field *f;

	while ((c = *arg) != '\0' && (c == ',' || isspace(c)))
		arg++;
	if (c == '\0')
		return (NULL);
	name = arg;
	arg = strpbrk(arg, " \t\r\v\f\n,=");
	if (arg != NULL) {
		c = *arg;
		*arg++ = '\0';
		if (c == '=') {
			char *s;

			header = arg;
			arg = NULL;
			width = strlen(header);
			s = header + width;
			while (s > header && isspace(*--s))
				*s = '\0';
			while (isspace(*header))
				header++;
		}
	}
	for (df = &fname[0]; df < &fname[NFIELDS]; df++)
		if (strcmp(name, df->fname) == 0)
			break;

	if (df >= &fname[NFIELDS]) {
		(void) fprintf(stderr,
			"ps: unknown output format: -o %s\n",
			name);
		errflg++;
		return (arg);
	}
	if ((f = (struct field *)malloc(sizeof (*f))) == NULL) {
		perror("malloc() for output format failed");
		exit(1);
	}
	f->next = NULL;
	f->fname = df - &fname[0];
	f->header = header? header : df->header;
	if (width == 0)
		width = df->width;
	if (*f->header != '\0')
		do_header = 1;
	f->width = max(width, df->minwidth);

	if (fields == NULL)
		fields = last_field = f;
	else {
		last_field->next = f;
		last_field = f;
	}

	return (arg);
}

static void
#ifdef __STDC__
pr_fields(struct pst_status *psinfo,
	void (*print_fld)(struct pst_status *, struct field *))
#else
pr_fields(psinfo, print_fld)
 struct pst_status *psinfo;
 void (*print_fld)();
#endif
{
	struct field *f;

	for (f = fields; f != NULL; f = f->next) {
		print_fld(psinfo, f);
		if (f->next != NULL)
			(void) printf(" ");
	}
	(void) printf("\n");
}

static void
#ifdef __STDC__
print_field(struct pst_status *psinfo, struct field *f)
#else
print_field(psinfo, f)
 struct pst_status *psinfo;
 struct field *f;
#endif
{
	int width = f->width;
	struct passwd *pwd;
	struct group *grp;
	struct sched_param param;
	time_t cputime;
	int scheduler;
	char *cp;
	ulong_t mask;
	char *s;

	switch (f->fname) {
	case F_ADDR:
		(void) printf("%*lx", width, (long)psinfo->pst_addr);
		break;
	case F_ARGS:
		if (xflg)
			if (pstat(PSTAT_GETCOMMANDLINE, pu, ARG_MAX, 1, psinfo->pst_pid) == -1) {
				perror("pstat_getcommandline");
				exit(1);
			}
		(void) printf("%s", xflg ? pu.pst_command : psinfo->pst_cmd);
		break;
	case F_CLASS:
		scheduler = (int)psinfo->pst_schedpolicy;
		switch(scheduler) {
		case PS_TIMESHARE:
			(void) printf("%-*.*s", width, width, "TIMESHARE");
			break;
		case PS_RTPRIO:
			(void) printf("%-*.*s", width, width, "RTPRIO");
			break;
		case PS_FIFO:
			(void) printf("%-*.*s", width, width, "FIFO");
			break;
		case PS_RR:
			(void) printf("%-*.*s", width, width, "RR");
			break;
		case PS_RR2:
			(void) printf("%-*.*s", width, width, "RR2");
			break;
		case PS_NOAGE:
			(void) printf("%-*.*s", width, width, "NOAGE");
			break;
		default:
			(void) printf("%-*.*s", width, width, "????");
		}
		break;
	case F_SCHED:
		if ((scheduler = sched_getscheduler(psinfo->pst_pid)) == -1) {
			(void) printf("%*.*s", width, width, "N-A");
			break;
		}
		switch(scheduler) {
		case SCHED_FIFO:
			(void) printf("%-*.*s", width, width, "SCHED_FIFO");
			break;
		case SCHED_RR:
			(void) printf("%-*.*s", width, width, "SCHED_RR");
			break;
		case SCHED_RR2:
			(void) printf("%-*.*s", width, width, "SCHED_RR2");
			break;
		case SCHED_RTPRIO:
			(void) printf("%-*.*s", width, width, "SCHED_RTPRIO");
			break;
		case SCHED_NOCHANGE:
			(void) printf("%-*.*s", width, width, "SCHED_NOCHANGE");
			break;
		case SCHED_NOAGE:
			(void) printf("%-*.*s", width, width, "SCHED_NOAGE");
			break;
		case SCHED_HPUX:
			(void) printf("%-*.*s", width, width, "SCHED_HPUX");
			break;
		default:
			(void) printf("%-*.*s", width, width, "N-A");
		}
		break;
	case F_COMM:
		if (s = strtok(psinfo->pst_cmd, " \t"))
			(void) printf("%-*s", width, basename(s));
		else
			(void) printf("%-*s", width, basename(psinfo->pst_cmd));
		break;
	case F_C:
		(void) printf("%*d", width, (int)psinfo->pst_cpu); // changed
		break;
	case F_ETIME:
		//print_time(delta_secs(&psinfo->pst_start), width);
		printf("N-A");
		break;
	case F_F:
		/*
		mask = 0xffffffffUL;
		if (width < 8)
			mask >>= (8 - width) * 4;
		(void) printf("%*lx", width, psinfo->pst_flag & mask);
		*/
		break;
	case F_GID:
		(void) printf("%*d", width, (int)psinfo->pst_egid);
		break;
	case F_GROUP:
		//if ((grp = getgrgid(psinfo->pst_egid)) != NULL)
		//	(void) printf("%*s", width, grp->gr_name);
		//else
			(void) printf("%*d", width, (int)psinfo->pst_egid);
		break;
	case F_NICE:
		(void) printf("%*d", width, (int)psinfo->pst_nice); // changed
		break;
	case F_PCPU:
		prtpct(psinfo->pst_pctcpu, width);
		break;
	case F_PGID:
		(void) printf("%*d", width, (int)psinfo->pst_pgrp);
		break;
	case F_PID:
		//(void) printf("%*d", width, (int)psinfo->pst_pid);
		(void) printf("%*lld", width, psinfo->pst_pid);
		break;
	case F_PPID:
		(void) printf("%*d", width, (int)psinfo->pst_ppid);
		break;
	case F_PRI:
		if ((scheduler = sched_getscheduler(psinfo->pst_pid)) == -1) {
			(void) printf("%*d", width, (int)psinfo->pst_pri); // changed
			break;
		}
		if (sched_getparam(psinfo->pst_pid, &param) == -1) {
			(void) printf("%*d", width, (int)psinfo->pst_pri); // changed
			break;
		}
		switch(scheduler) {
		case SCHED_FIFO:
		case SCHED_RR:
		case SCHED_RR2:
			(void) printf("%*i", width, param.sched_priority);
			break;
		default:
			(void) printf("%*d", width, (int)psinfo->pst_pri); // changed
		}
		break;
	//case F_PRMID: 
	//case F_PRMGRP: 
	case F_RGID:
		(void) printf("%*d", width, (int)psinfo->pst_gid);
		break;
	case F_RGROUP:
		(void) printf("%*d", width, (int)psinfo->pst_gid);
		break;
	case F_RUID:
		(void) printf("%*d", width, (int)psinfo->pst_uid);
		break;
	//case F_PSET:
	case F_RUSER:
		if ((pwd = getpwuid(psinfo->pst_uid)) != NULL)
			(void) printf("%*s", width, pwd->pw_name);
		else
			(void) printf("%*d", width, (int)psinfo->pst_uid);
		break;
	case F_SID:
		(void) printf("%*d", width, (int)psinfo->pst_sid);
		break;
	case F_S:
/* States from ps(1):
	   0    Nonexistent		I    Intermediate
	   S    Sleeping		Z    Terminated
	   W    Waiting			T    Stopped
	   R    Running			X    Growing
*/
		switch((int)psinfo->pst_stat) {
		case PS_SLEEP:
			(void) printf("%.*s", width, "SLEEP");
			break;
		case PS_RUN:
			(void) printf("%.*s", width, "RUN");
			break;
		case PS_STOP:
			(void) printf("%.*s", width, width > 1 ? "STOPPED" : "T");
			break;
		case PS_ZOMBIE:
			(void) printf("%.*s", width, "ZOMBIE");
			break;
		case PS_IDLE:
			(void) printf("%.*s", width, "IDLE");
			break;
		case PS_OTHER:
			(void) printf("%.*s", width, "OTHER");
			break;
		default:
			(void) printf("%.*s", width, "0");
		}
		break;
	case F_SZ:
		(void) printf("%*lu", width, (ulong_t)
			(psinfo->pst_dsize + 
			 psinfo->pst_tsize + 
			 psinfo->pst_ssize) * page_size / sc_page_size);
		break;
	case F_TIME:
		cputime = psinfo->pst_time;
		if (psinfo->pst_time > 500000000)
			cputime++;
		print_time(cputime, width);
		break;
	case F_TTY:
		(void) printf("%-*s", width, "N-A");
		break;
	case F_UID:
		(void) printf("%*d", width, (int)psinfo->pst_euid);
		break;
	case F_USER:
		if ((pwd = getpwuid(psinfo->pst_euid)) != NULL)
			(void) printf("%*s", width, pwd->pw_name);
		else
			(void) printf("%*d", width, (int)psinfo->pst_euid);
		break;
	case F_VSZ:
		(void) printf("%*lu", width, (ulong_t)
			((psinfo->pst_vtsize + 
			 psinfo->pst_vdsize + 
			 psinfo->pst_vssize) * page_size / 1024));
		break;
	case F_WCHAN:
		(void) printf("%*lx", width, (long)psinfo->pst_wchan);
		break;
	}
}
static void
#ifdef __STDC__
print_time(time_t tim, int width)
#else
print_time(tim, width)
 time_t tim; 
 int width;
#endif
{
	char buf[30];
	time_t seconds;
	time_t minutes;
	time_t hours;
	time_t days;

	if (tim < 0) {
		(void) printf("%*s", width, "-");
		return;
	}

	seconds = tim % 60;
	tim /= 60;
	minutes = tim % 60;
	tim /= 60;
	hours = tim % 24;
	days = tim / 24;

	if (days > 0) {
		(void) snprintf(buf, sizeof (buf), "%ld-%2.2ld:%2.2ld:%2.2ld",
		    days, hours, minutes, seconds);
	} else if (hours > 0) {
		(void) snprintf(buf, sizeof (buf), "%2.2ld:%2.2ld:%2.2ld",
		    hours, minutes, seconds);
	} else {
		(void) snprintf(buf, sizeof (buf), "%2.2ld:%2.2ld",
		    minutes, seconds);
	}

	(void) printf("%*s", width, buf);
}

static void
#ifdef __STDC__
prtpct(float pct, int width)
#else
prtpct(pct, width)
 float pct;
 int width;
#endif
{
	float value = pct * 100;

	if (value >= 100)
		(void) printf("%*.f", width, value);
	else if (value >= 10)
		(void) printf("%*.1f", width, value);
	else
		(void) printf("%*.2f", width, value);
}

