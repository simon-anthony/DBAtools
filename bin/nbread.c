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
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <poll.h>
#include <libgen.h>

#define USAGE "[-t <timeout>]"
#define MS 1000

#ifdef linux
#define POLLNORM POLLRDNORM
#include <string.h>
#endif

char *prog;

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
 int argc;
 char *argv[];
#endif
{
	extern char *optarg;
	extern int optind, opterr, optopt;
	char buf[BUFSIZ], ch;
	struct pollfd fds;
	int  timeout = 10000;
	int  c;
	int  n;
	int nr, nw;
	int tflg = 0, errflg = 0;

	prog = strdup(basename(argv[0]));

	while ((c = getopt(argc, argv, "t:")) != -1)
		switch(c) {
		case 't':
			tflg++;
			if ((timeout = atoi(optarg)) == 0) 
				errflg++;
			else
				timeout = timeout * MS;
			break;
		case ':':
			fprintf(stderr, "%s: option -%c requires an argument\n", prog, optopt);
			errflg++;
			break;
		case '?':
			fprintf(stderr, "%s: unrecognized option: - %c\n", prog,  optopt);
			errflg++;
		}
	if (argc - optind != 0)
		errflg++;

	if (errflg) {
		fprintf(stderr, "usage: %s %s\n", prog, USAGE);
		exit(2);
	}

	fds.fd = STDIN_FILENO;
	fds.events = POLLNORM;
	
	if ((n = poll(&fds, 1, timeout)) <= 0 ) {
		if (n == -1) {
			perror("poll");
			exit(1);
		}
		exit(1);
	}

	if (!fds.revents&POLLIN)
		exit(1);

	nr = 0;
	while ((n = (read(STDIN_FILENO, &ch, 1)) == 1) && nr < BUFSIZ) {
		if (ch == '\n') {
			if ((nw = write(STDOUT_FILENO, (void *)buf, nr)) != nr) {
			    perror("write");
				exit(1);
			}
			exit(0);
		}
		buf[nr++] = ch;
	}
	exit(1);
}
