#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid = "$Header$";
#endif
/* Copyright (C) 2012 Simon Anthony
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
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/pstat.h>
#include <limits.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

extern char *prog;

static struct pst_status *pst = NULL;
static struct pst_dynamic psd;
static int nproc;	/* active processes */

#define TABSIZE 1000
static size_t nel = 0;
static _T_LONG_T tab[TABSIZE];
_T_LONG_T *lsearch();

static int
compar(_T_LONG_T *a, _T_LONG_T *b)
{
	if (*a == *b)
		return 0;
	if (*a < *b)
		return -1;
	return 1;
}

int terse = 0;		/* available externally */
int extended = 0;
int noresolve = 0;
int nofqdn = 0;

#define MAX_LENGTH 1024
static char long_command[MAX_LENGTH];
static union pstun pu;

#ifdef __STDC__
extern int ckuser(struct pst_status *);
#else
extern int ckuser();
#endif

char *
#ifdef __STDC__
getname(struct sockaddr_in *sa, int nolookup, int nofqdn)
#else
getname(sa, nolookup, nofqdn)
 struct sockaddr_in *sa;
 int nolookup;
 int nofqdn;
#endif
{
	static char	hbuf[NI_MAXHOST];
	int		err;
	char	*s;

	if (nolookup)
		goto addr;

	if ((err = getnameinfo((struct sockaddr *)sa, sizeof(sa),
		hbuf, sizeof(hbuf), NULL, 0, (nofqdn ? NI_NOFQDN :0) | NI_NAMEREQD)) != 0) {
		if (err == EAI_NONAME) { /* does not resolve - don't care */
			goto addr;
		}

		fprintf(stderr, "error using getnameinfo: %s\n",
			gai_strerror(err));

		return inet_ntoa(sa->sin_addr);
	}
		
	return (char *)hbuf;
addr:
	s = inet_ntoa(sa->sin_addr);

	if (strcmp(s, "0.0.0.0") == 0)
		return "*";
	return s;
	
}

#define BURST ((size_t)10)
static int
#ifdef __STDC__
getprocports(struct pst_status *p, int port)
#else
getprocports(p, port)
 struct pst_status *p;
 int port;
#endif
{
	struct pst_fileinfo2 psf[BURST];
	struct pst_socket psfsocket;
	char filename[PATH_MAX];
	int i, count;
	int idx = 0; /* index within the context */
	int	nports = 0;
	static struct passwd *pw;


	/* loop until all fetched */
	while ((count = pstat_getfile2(psf, sizeof(struct pst_fileinfo2), BURST, idx, p->pst_pid)) > 0) {
		/* process them (max of BURST) at a time */
		for (i = 0; i < count; i++) {
			if (psf[i].psf_ftype == PS_TYPE_SOCKET) {
				if (pstat_getsocket(&psfsocket, sizeof(struct pst_socket), &(psf[i].psf_fid)) != 0) {
					if ((psfsocket.pst_hi_fileid == psf[i].psf_hi_fileid) 
						 &&	(psfsocket.pst_lo_fileid == psf[i].psf_lo_fileid)
						 &&	(psfsocket.pst_hi_nodeid == psf[i].psf_hi_nodeid)
						 &&	(psfsocket.pst_lo_nodeid == psf[i].psf_lo_nodeid)) {
						if ((psfsocket.pst_family == PS_AF_INET || psfsocket.pst_family == PS_AF_INET6) &&
							psfsocket.pst_protocol == PS_PROTO_TCP &&
							psfsocket.pst_pstate == (unsigned int)PS_TCPS_LISTEN) { // changed
							nports++;
							if ((size_t)psfsocket.pst_boundaddr_len == sizeof(struct sockaddr_in)) {
								if (port) 
									if ((int)(((struct sockaddr_in *)psfsocket.pst_boundaddr)->sin_port) != port)
										continue;
								if (!ckuser(p))
									continue;
								if (terse) {
									/* filter out duplicate PIDs */
									if (nel < TABSIZE) 
										(void *) lsearch((char *)&p->pst_pid, (char *)tab, &nel, sizeof(_T_LONG_T), compar);
									
									continue;
								}
								pw = getpwuid(p->pst_uid);
								if (extended) {
									if (pstat(PSTAT_GETCOMMANDLINE, pu, MAX_LENGTH, 1, p->pst_pid) == -1) {
										perror("pstat");
										return(-1);
									}
									printf("%5lld %7s %5d %s\n",
										p->pst_pid,
										pw ? pw->pw_name : ltoa(p->pst_uid),
										(int)(((struct sockaddr_in *)psfsocket.pst_boundaddr)->sin_port),
										extended ? pu.pst_command : p->pst_cmd);
								} else
									printf("%-8s %5lld %7s %s:%d\n",
										p->pst_ucomm, 
										p->pst_pid,
										pw ? pw->pw_name : ltoa(p->pst_uid),
										getname((struct sockaddr_in *)psfsocket.pst_boundaddr, noresolve, nofqdn),
										(int)htons(((struct sockaddr_in *)psfsocket.pst_boundaddr)->sin_port)
									);
							}
						}
					}
				}
			}
		}
		/*
		 * Now go back and do it again, using the
		 * next index after the current 'burst'
		 */
		idx = psf[count-1].psf_fd + 1;
	}
	//if (count == -1)
	//	 perror("pstat_getfile2()"); // probably no such process

#undef BURST
	return nports;
}

void
#ifdef __STDC__
getports(int port)
#else
getports(port)
 int port;
#endif
{
	char	*buf[BUFSIZ];
	int		i;
	struct pst_status *p = NULL;

	if (!pst) {
		if (pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0) == -1) {
			perror("pstat_getdynamic");
			exit(1);
		}
		if ((pst = malloc(
				sizeof(struct pst_status) * psd.psd_maxprocs)) == NULL) {
			perror("malloc");
			exit(1);
		}
		if ((nproc = pstat_getproc(
				pst,
				sizeof(struct pst_status),
				psd.psd_activeprocs, 0)) != -1) { 
		} else {
			perror("pstat_getproc");
			exit(1);
		}
	}
	if (getenv("UNIX95"))
		extended = 1;
	if (extended)
		pu.pst_command = long_command;

	if (!terse) {
		if (extended)
			printf("%5s %7s %5s %s\n", "PID", "USER", "PORT", "COMMAND");
		else
			printf("%-8s %5s %7s %-s\n", "COMMAND", "PID", "USER", "PORT");
	}
	for (i = 0, p = pst; i < nproc; i++, p++) {
		getprocports(p, port);
	}
	if (terse)
		for (i = 0; i < nel; i++)
			printf("%lld\n", tab[i]);
}
