/*
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
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#include <chunk.h>
#include <trade_msg_la.h>

#include "input.h"
#include "dbg.h"

#define BUFSIZE 65536*8

struct input_desc {
  int fd;
};

struct input_desc *input_open(const char *fname, int *fds, int fds_size)
{
  struct input_desc *res;

  res = malloc(sizeof(struct input_desc));
  if (res == NULL) {
    return NULL;
  }
  res->fd = -1;

  if (!fname){
    res->fd = STDIN_FILENO;
  } else {
    char *c;
    int port;
    char ip[32];

    c = strchr(fname,',');
    if (c) {
      *(c++) = 0;
    }

    if (sscanf(fname,"tcp://%[0-9.]:%d", ip, &port) == 2) {

      int accept_fd;

      accept_fd = socket(AF_INET, SOCK_STREAM, 0);
      if (accept_fd < 0) {
        fprintf(stderr,"Error creating socket\n");
      } else {
        struct sockaddr_in servaddr;

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	//TODO
        servaddr.sin_port = htons(port);
        if (bind(accept_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
          fprintf(stderr,"Error binding to socket\n");
        } else if (listen(accept_fd, 1) < 0) {
          fprintf(stderr,"Waiting for chunkstream connection\n");
        } else {
          res->fd = accept(accept_fd, NULL, NULL);
          if (res->fd < 0) {
            fprintf(stderr,"Error accepting connection\n");
          }
        }
        close(accept_fd);
      }
    } else {
      res->fd = open(fname, O_RDONLY); //TODO
    }
  }

  if (res->fd < 0) {
    free(res);
    return NULL;
  } else {
#ifndef _WIN32
    if (fcntl(res->fd, F_SETFL, O_NONBLOCK) == -1) {
      perror("fcntl(O_NONBLOCK)");
    }
#else
    unsigned long nonblocking = 1;
    ioctlsocket(res->fd, FIONBIO, (unsigned long*) &nonblocking);
#endif
  }

  fds[0] = res->fd;
  fds[1] = -1;

  return res;
}

void input_close(struct input_desc *s)
{
  close(s->fd);
  free(s);
}

int input_get(struct input_desc *s, struct chunk *c)
{
  int ret;
  uint32_t size;
  static char recvbuf[BUFSIZE];
  static int pos = 0;

  c->data = NULL;
  ret = recv(s->fd, recvbuf + pos, BUFSIZE - pos, 0);
  if (ret < 0) {
#ifndef _WIN32
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
#else
    if (WSAGetLastError() != WSAEWOULDBLOCK) {
#endif
      perror("chunkstream connection error");
      exit(EXIT_FAILURE);
    }
  } else {
    pos += ret;
  }


  if ( pos < sizeof(size)) { // not enough data to decode chunk size
    if (ret == 0) {	// connection closed
      return -1;
    } else {
      return INT_MAX;
    }
  }


  size = ntohl(*(uint32_t*)recvbuf);
  if (ret == 0 && pos < sizeof(size) + size) { // connection closed, not enough data to decode last chunk
      return -1;
  }
  if (pos >= sizeof(size) + size) {
    ret = decodeChunk(c, recvbuf + sizeof(size), size);
    if (ret < 0) {
      printf("Error decoding chunk!\n");
      return -1;
    }

    // remove attributes //TODO: verify whether this is the right place to do this
    if (c->attributes) {
      free(c->attributes);
      c->attributes = NULL;
      c->attributes_size = 0;
    }

    pos -= sizeof(size) + size;
    memmove(recvbuf, recvbuf + sizeof(size) + size, pos);
  }

  //check if there are other chunks in the buffer
  if ( pos >= sizeof(size)) {
    size = ntohl(*(uint32_t*)recvbuf);
    if (pos >= sizeof(size) + size) {
      return 0;
    }
  }

  return INT_MAX;
}
