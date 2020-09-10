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
 * gethost(), sethost() - like getpwent() setpwent() for hostnames.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef __sun
#define BSD_COMP
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#ifdef __linux
#include <linux/if.h>
#else
#include <net/if.h>
#endif
#include <netdb.h>

#ifndef   NI_MAXHOST
#define   NI_MAXHOST 1025
#endif // NI_MAX_HOST
 
// On a DARWIN system EAI_OVERFLOW is define as EAI_MAX
#ifndef   EAI_OVERFLOW
#define   EAI_OVERFLOW EAI_MAX
#endif // EAI_OVERFLOW

#define MAXHOSTS	20
#define	BUFSZ		256
static char *hosts[MAXHOSTS];
static int nhost = 0;
static char **p = NULL;

// takes the given sockaddr pointer, turns it into a sockaddr_in pointer,
// grabs the sin_addr member from it, and grabs the submember s_addr,
// and changes i to its pointer before turning it into an unsigned char
// to assign to the pointer var. 
/*
void inline
#ifdef __STDC__
printv4ip(const struct sockaddr *const addr)
#else
printv4ip(addr)
struct sockaddr *addr
#endif
{
	uint8_t *const poctet =
		(uint8_t *) &(((struct sockaddr_in *) addr)->sin_addr.s_addr);

	printf("Address: %hhu.%hhu.%hhu.%hhu\n",
		*poctet, *(poctet + 1), *(poctet + 2), *(poctet + 3));
 
	return;
}
*/

#ifdef __STDC__
void sethost(void)
#else
void sethost()
#endif
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;
	int  nifaces, i;
	int sock;
	char hbuf[NI_MAXHOST];
	int err;

	nhost = 0;

	memset(&ifconf, 0, sizeof(ifconf));

	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	/* Get a socket for the icotl request */
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		perror("socket");
		return;
	}

	/* SIOCGCONF only fills in name ifr_name and ifr_addr */
	if (ioctl(sock, SIOCGIFCONF, &ifconf) < 0) {
		perror("ioctl(SIOGIFCONF)");
		return;
	}

	nifaces =  ifconf.ifc_len / sizeof(struct ifreq);

	/* printf("Interfaces (count = %d):\n", nifaces); */

	for (i = 0; i < nifaces; i++) {
		struct ifreq ifr;
		char *p1;

		strcpy(ifr.ifr_name, ifreqs[i].ifr_name);

		/* Get flags with new request using SIOCGIFFLAGS */
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
			perror("ioctl(SIOCGIFFLAGS)");
			continue;
		}

		if (ifr.ifr_flags & IFF_LOOPBACK) /* not interested in this */
			continue;

		if ((err = getnameinfo(&ifreqs[i].ifr_addr, sizeof(ifreqs[i].ifr_addr),
			hbuf, sizeof(hbuf), NULL, 0, NI_NOFQDN | NI_NAMEREQD)) != 0) {
			if (err == EAI_NONAME) /* does not resolve - don't care */
				continue;

			fprintf(stderr, "error using getnameinfo: %s\n",
				gai_strerror(err));
			continue;
		}
		if (hosts[nhost]) free(hosts[nhost]);
		/* On some platforms NI_FNOFQDN does not work, e.g. OS-X */
		if ((p1 = strchr(hbuf, '.')) != NULL) *p1 = '\0';

		hosts[nhost++] = strdup(hbuf);
	}

	close(sock);
	if (hosts[nhost]) free(hosts[nhost]);
	hosts[nhost] = NULL;
	p = hosts;
}

#ifdef __STDC__
char *gethost(void)
#else
char *gethost()
#endif
{
	if (!*p) {
		if (nhost == 0)
			sethost();
		else
			return NULL;
	}
	return(*p ? *p++ : NULL);
}
