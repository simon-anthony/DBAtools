#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#define PASSWORD "Polish!1"

int
main(int argc, char *argv[])
{
	char *host, *ps1;
	size_t len = sysconf(_SC_HOST_NAME_MAX);
	int fd;
	char buf[BUFSIZ];
	struct termios old, new;

	if ((fd = open("/dev/tty", O_RDWR)) < 0) {
		perror("open");
		exit(1);
	}

	if (tcgetattr(fd, &old) < 0) {
		perror("tcgetattr");
		exit(1);
	}
	new = old;

	new.c_lflag &= ~ECHO;
	
	if (tcsetattr(fd, TCSAFLUSH, &new) < 0) {
		perror("tcsetattr");
		exit(1);
	}

	write(fd, "Password: ", 10);
	read(fd, (char *)buf, BUFSIZ);

	if (tcsetattr(fd, 0, &old) < 0) {
		perror("tcsetattr");
		exit(1);
	}

	close(fd);

	if (strncmp((char *)buf, PASSWORD, strlen(PASSWORD)) != 0) {
		fprintf(stderr, "Sorry!\n");
		exit(1);
	}
	putchar('\n');

	if (setreuid(0, -1) != 0) {
		perror("setuid");
		exit(1);
	}
	
	if ((host = (char *)malloc(len)) != NULL)
		if (gethostname(host, len) == 0) 
			if ((ps1 = (char *)malloc(len + 7)) != NULL) {
				strcpy(ps1, "PS1=");
				strcat(ps1, host);
				free(host);
				strcat(ps1, "# ");
				putenv(ps1);
				free(ps1);
			}
	execl(getenv("SHELL"), getenv("SHELL"), (char *)NULL);

	perror("execl");
	exit(1);
}
