#ifdef __IBMC__
#pragma comment (user, "$Header$")
#else
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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>

#ifndef   NI_MAXHOST
#define   NI_MAXHOST 1025
#endif
 
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define SIZE(p) MAX((p).sa_len, sizeof(p))

#define MAXHOSTS	20
#define	BUFSZ		256
static char *hosts[MAXHOSTS];
static int nhost = 0;
static char **pp = NULL;

void sethost(void)
{
	struct ifreq *ifr;
	struct ifconf ifconf;
	int		sock;
	char	hbuf[NI_MAXHOST];
	int		err;
	int		size = 1;

	nhost = 0;
	/* Get a socket for the icotl request */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
		perror("socket");
		return;
	}

	if (ioctl(sock, SIOCGSIZIFCONF, &size) < 0) {
		perror("ioctl(SIOCGSIZIFCONF)");
		return;
	}
	ifconf.ifc_req = (struct ifreq *)malloc(size);
	ifconf.ifc_len = size;

	/* SIOCGIFCONF only fills in name ifr_name and ifr_addr */
	if (ioctl(sock, SIOCGIFCONF, &ifconf) < 0) {
		perror("ioctl(SIOCGIFCONF)");
		return;
	}
	ifr = ifconf.ifc_req;

	while ((char *)ifr < (char *) ifconf.ifc_req + ifconf.ifc_len) {
		struct ifreq flg;

		memcpy(flg.ifr_name, ifr->ifr_name, sizeof(flg.ifr_name));

		if (ioctl(sock, SIOCGIFFLAGS, &flg) < 0) {
			perror("ioctl(SIOCGIFFLAGS)");
			goto next;
		}

		if (flg.ifr_flags & IFF_LOOPBACK) /* not interested in this */
			goto next;
		if (ifr->ifr_addr.sa_family != AF_INET && ifr->ifr_addr.sa_family != AF_INET6) /* not interested in this */
			goto next;

		if ((err = getnameinfo(&(ifr->ifr_addr), sizeof(struct sockaddr), hbuf, sizeof(hbuf), NULL, 0, NI_NOFQDN | NI_NAMEREQD)) != 0) {
			if (err == EAI_NONAME) /* does not resolve - don't care */
				goto next;

			fprintf(stderr, "error using getnameinfo: %s\n", gai_strerror(err));
			goto next;
		}
		if (hosts[nhost]) free(hosts[nhost]);

		hosts[nhost++] = strdup(hbuf);

	next:
		ifr = ((char *)ifr + sizeof(ifr->ifr_name) + SIZE(ifr->ifr_addr));
	}

	close(sock);
	if (hosts[nhost]) free(hosts[nhost]);
	hosts[nhost] = NULL;
	pp = hosts;
}

#ifdef __STDC__
char *gethost(void)
#else
char *gethost()
#endif
{
	if (!*pp) {
		if (nhost == 0)
			sethost();
		else
			return NULL;
	}
	return(*pp ? *pp++ : NULL);
}
