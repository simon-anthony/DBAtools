#ifdef __SUN__
#pragma ident "$Header$"
#endif
#ifdef __IBMC__
#pragma comment (user, "$Header$")
#endif
#ifdef _HPUX_SOURCE
static char *svnid_lock = "$Header$";
#endif
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
/*
 * lock: do advisory locking on fd
 */
int
#ifdef __STDC__
lock(char *path)
#else
lock(path)
 char *path;
#endif
{
	int fd, val;
	struct flock flk;
	char	buf[10];

	flk.l_type = F_WRLCK;
	flk.l_start = 0;
	flk.l_whence = SEEK_SET;
	flk.l_len = 0;

	if ((fd = open(path, O_RDWR)) < 0) {
		fprintf(stderr, "oavar: cannot open '%s' for read/write\n", path);
		exit(1);
	}
	
	/* attempt lock of entire file */

	if (fcntl(fd, F_SETLK, &flk) < 0) {
		if (errno == EACCES || errno == EAGAIN) {
			fprintf(stderr, "oavar: file '%s' being edited try later\n", path);
			exit(0);	/* gracefully exit */
		} else {
			fprintf(stderr, "oavar: error in attempting lock of '%s'\n", path);
			exit(1);
		}
	}

	/* set close-on-exec flag for fd */

	if ((val = fcntl(fd, F_GETFD, 0)) < 0) {
		perror("getfd");
		exit(1);
	}
	val |= FD_CLOEXEC;

	if (fcntl(fd, F_SETFD, val) < 0) {
		perror("setfd");
		exit(1);
	}

	/* leave file open until we terminate: lock will be held */

	return fd;
}

void
#ifdef __STDC__
fdunlock(int fd)
#else
fdunlock(int)
 char *fd;
#endif
{
	struct flock flk;
	char	buf[10];

	flk.l_type = F_UNLCK;
	flk.l_start = 0;
	flk.l_whence = SEEK_SET;
	flk.l_len = 0;

	/* attempt lock of entire file */

	if (fcntl(fd, F_SETLK, &flk) < 0) {
		if (errno == EACCES || errno == EAGAIN) {
			fprintf(stderr, "oavar: unlock failure\n");
			exit(0);	/* gracefully exit */
		} else {
			perror("fcntl");
			fprintf(stderr, "oavar: error in attempting unlock\n");
			exit(1);
		}
	}
}
