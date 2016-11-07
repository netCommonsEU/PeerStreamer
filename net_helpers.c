/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
 *
 * This file is part of PeerStreamer.
 *
 * PeerStreamer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PeerStreamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PeerStreamer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <sys/types.h>
#ifndef _WIN32
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>     /* For struct ifreq */
#include <netdb.h>
#else
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* WINNT>=0x501 (WindowsXP) for supporting getaddrinfo/freeaddrinfo.*/
#endif
#include <ws2tcpip.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net_helpers.h"
extern enum L3PROTOCOL {IPv4, IPv6} l3;

char *iface_addr(const char *iface)
{
#ifndef _WIN32
    struct ifaddrs *if_addr, *ifap;
	int family;
	char *host_addr;
	int ifcount;

	if (getifaddrs(&if_addr) == -1)
	{
	  perror("getif_addrs");
	  return NULL;
	}
	ifcount = 0;
	for (ifap = if_addr; ifap != NULL; ifap = ifap->ifa_next)
	{
		if (ifap->ifa_addr == NULL)
		{
			ifcount++;
			continue;
		}
		family = ifap->ifa_addr->sa_family;
		if (l3 == IPv4 && family == AF_INET && !strcmp (ifap->ifa_name, iface))
		{
			host_addr = malloc((size_t)INET_ADDRSTRLEN);
			if (!host_addr)
			{
				perror("malloc host_addr");
				return NULL;
			}
			if (!inet_ntop(AF_INET, (void *)&(((struct sockaddr_in *)(ifap->ifa_addr))->sin_addr), host_addr, INET_ADDRSTRLEN))
			{
				perror("inet_ntop");
				return NULL;
			}
			break;
		}
		if (l3 == IPv6 && family == AF_INET6 && !strcmp (ifap->ifa_name, iface))
		{
			host_addr = malloc((size_t)INET6_ADDRSTRLEN);
			if (!host_addr)
			{
				perror("malloc host_addr");
				return NULL;
			}
			if (!inet_ntop(AF_INET6, (void *)&(((struct sockaddr_in6 *)(ifap->ifa_addr))->sin6_addr), host_addr, INET6_ADDRSTRLEN))
			{
				perror("inet_ntop");
				return NULL;
			}
			break;
		}

	}
	freeifaddrs(if_addr);
	return host_addr;
#else
    char *res;
    if(iface != NULL && strcmp(iface, "lo") == 0)
    {
	  switch (l3)
	  {
		case IPv4:
			res = malloc (INET_ADDRSTRLEN);
			strcpy(res, "127.0.0.1");
		  break;
		case IPv6:
			res = malloc (INET6_ADDRSTRLEN);
			strcpy(res, "::1");
		  break;
		default:
		  return NULL;
		  break;
	  }
      return res;
    }
    if(iface != NULL && inet_addr(iface) != INADDR_NONE) return strdup(iface);
    return default_ip_addr();
#endif
}

const char *hostname_ip_addr()
{
#ifndef _WIN32
  const char *ip;
  char hostname[256];
  struct addrinfo * result;
  struct addrinfo * res;
  int error;

  if (gethostname(hostname, sizeof hostname) < 0) {
    fprintf(stderr, "can't get hostname\n");
    return NULL;
  }
  fprintf(stderr, "hostname is: %s ...", hostname);

  /* resolve the domain name into a list of addresses */
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0) {
    fprintf(stderr, "can't resolve IP: %s\n", gai_strerror(error));
    return NULL;
  }

  /* loop over all returned results and do inverse lookup */
  for (res = result; res != NULL; res = res->ai_next) {
    ip = inet_ntoa(((struct sockaddr_in*)res->ai_addr)->sin_addr);
    fprintf(stderr, "IP is: %s ...", ip);
    if ( strncmp("127.", ip, 4) == 0) {
      fprintf(stderr, ":( ...");
      ip = NULL;
    } else {
      break;
    }
  }
  freeaddrinfo(result);

  return ip;
#else
  const char *ip;
  char hostname[256];
  struct addrinfo hints, *result, *res;
  int error;
  ip = malloc (INET6_ADDRSTRLEN);
  if (!ip)
  {
	  perror("hostname_ip_addr");
	  return NULL;
  }
  fprintf(stderr, "Trying to guess IP ...");
  if (gethostname(hostname, sizeof hostname) < 0) {
    fprintf(stderr, "can't get hostname\n");
    return NULL;
  }
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = AF_UNSPEC;
  hints.ai_protocol = IPPROTO_TCP;

  fprintf(stderr, "hostname is: %s ...\n", hostname);
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0)
  {
    fprintf(stderr, "can't resolve IP: %s\n", gai_strerror(error));
    return NULL;
  }

  for (res = result; res != NULL ; res = res->ai_next)
  {
      fprintf(stderr, "Address Family is %d...", res->ai_family);
	  if ( (res->ai_family == AF_INET && l3 == IPv4) ||
			  (res->ai_family == AF_INET6 && l3 == IPv6))
	  {
		  if (res->ai_family == AF_INET)
		  {
			  inet_ntop(res->ai_family, &((const struct sockaddr_in *)(res->ai_addr))->sin_addr, ip, INET6_ADDRSTRLEN);
		  }
		  else
		  {
			  inet_ntop(res->ai_family, &((const struct sockaddr_in6 *)(res->ai_addr))->sin6_addr, ip, INET6_ADDRSTRLEN);
		  }
		  fprintf(stderr, "IP is: %s ...", ip);
		  if ( (l3 == AF_INET && strncmp("127.", ip, 4) == 0) ||
				  (l3 == AF_INET6 && strncmp("::1", ip, 3) == 0)) {
			  fprintf(stderr, ":( ...");
			  memset (&ip, 0, INET6_ADDRSTRLEN);
		  }
		  else break;
	  }
  }
  freeaddrinfo(result);

  return ip;
#endif
}

const char *autodetect_ip_address() {
#ifdef __linux__

//	static char addr[128] = "";
	char iface[IFNAMSIZ] = "";
	char line[128] = "x";
//	struct ifaddrs *ifaddr, *ifa;
//	char *host_addr;
//	int res;

	FILE *r = fopen("/proc/net/route", "r");
	if (!r) return NULL;

	while (1) {
		char dst[32];
		char ifc[IFNAMSIZ];

		fgets(line, 127, r);
		if (feof(r)) break;
		if ((sscanf(line, "%s\t%s", iface, dst) == 2) && !strcpy(dst, "00000000")) {
			strcpy(iface, ifc);
		 	break;
		}
	}

	return iface_addr(iface);
//	if (iface[0] == 0) return NULL;
//
//	if (getifaddrs(&ifaddr) < 0) {
//		perror("getifaddrs");
//		return NULL;
//	}
//
//	ifa = ifaddr;
//	while (ifa) {
//		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
//			ifa->ifa_name && !strcmp(ifa->ifa_name, iface))  {
//            if (l3 == IPv4 && ifa->ifa_addr->sa_family == AF_INET)
//            {
//        			host_addr = malloc((size_t)INET_ADDRSTRLEN);
//        			if (!host_addr)
//        			{
//        				perror("malloc host_addr");
//        				return NULL;
//        			}
//        			if (!inet_ntop(ifa->ifa_addr->sa_family, (void *)&(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr), host_addr, INET_ADDRSTRLEN))
//        			{
//        				perror("inet_ntop");
//        				return NULL;
//        			}
//        			break;
//        		}
//                void *tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
//
//                res = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), line, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
//                printf("dev: %-8s address: <%s> \n", ifa->ifa_name, line);
//                if (inet_ntop(AF_INET, tmpAddrPtr, addr, 127)) {
//                        ret = addr;
//                } else {
//                        perror("inet_ntop error");
//                        ret = NULL;
//                }
//                break;
//            }
//            if (l3 == IPv6 && ifa->ifa_addr->sa_family == AF_INET6){
//                void *tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
//                if (inet_ntop(AF_INET6, tmpAddrPtr, addr, 127)) {
//                      ret = addr;
//                } else {
//                        perror("inet_ntop error");
//                        ret = NULL;
//                }
//                break;
//            }
//			break;
//		}
//	ifa=ifa->ifa_next;
//	}
//
//	freeifaddrs(ifaddr);
//	return ret;
#else
		return hostname_ip_addr();
//        return simple_ip_addr();
#endif
}

char *default_ip_addr()
{
  const char *ip = NULL;

  fprintf(stderr, "Trying to guess IP ...");

  ip = autodetect_ip_address();

  if (!ip) {
    fprintf(stderr, "cannot detect IP!\n");
    return NULL;
  }
  fprintf(stderr, "IP is: %s ...\n", ip);

  return strdup(ip);
}
