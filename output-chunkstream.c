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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <chunk.h>
#include <chunkiser.h>
#include <trade_msg_la.h>

#include "measures.h"
#include "dbg.h"

#define BUFSIZE 65536*8
static int fd = -1;
static char *fname = NULL;
static enum MODE {FILE_MODE, TCP_MODE} mode;

#ifdef _WIN32
static int inet_aton(const char *cp, struct in_addr *addr)
{
    if( cp==NULL || addr==NULL )
    {
        return(0);
    }

    addr->s_addr = inet_addr(cp);
    return (addr->s_addr == INADDR_NONE) ? 0 : 1;
}
#endif

static void output_connect(void)
{
  if (!fname){
    mode = FILE_MODE;
    fd = STDOUT_FILENO;
  } else {
    char *c;
    int port;
    char ip[32];

    c = strchr(fname,',');
    if (c) {
      *(c++) = 0;
    }

    if (sscanf(fname,"tcp://%[0-9.]:%d", ip, &port) == 2) {

      mode = TCP_MODE;
      fd = socket(AF_INET, SOCK_STREAM, 0);
      if (fd < 0) {
        fprintf(stderr,"output-chunkstream: Error creating socket\n");
      } else {
        struct sockaddr_in servaddr;

        fprintf(stderr,"output-chunkstream: tcp socket opened fd=%d\n", fd);
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        if (inet_aton(ip, &servaddr.sin_addr) < 0) {
          fprintf(stderr,"output-chunkstream: Error converting IP address: %s\n", ip);
          return;
        }
        if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
          fprintf(stderr,"output-chunkstream: Error connecting to %s:%d\n", ip, port);
        } else {
          fprintf(stderr,"output-chunkstream: Connected to %s:%d\n", ip, port);
        }
      }
    } else {
      mode = FILE_MODE;
#ifndef _WIN32
      fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC | O_NONBLOCK, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#else
      fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC);
      if (fd >= 0) {
         unsigned long nonblocking = 1;
         ioctlsocket(fd, FIONBIO, (unsigned long*) &nonblocking);
      }
#endif
      if (fd < 0) {
        fprintf(stderr,"output-chunkstream: Error opening output file %s", fname);
        perror(NULL);
      } else {
        fprintf(stderr,"output-chunkstream: opened output file %s\n", fname);
      }
    }
  }
}

struct output_stream *out_stream_init(const char *fn, const char *config)
{
  if (fn) fname = strdup(fn);
  output_connect();
  return 1;	//TODO: create real output_stream structure
}

void chunk_write(struct output_stream *out, const struct chunk *c)
{
  static char sendbuf[BUFSIZE];
  static int pos = 0;
  int ret;
  uint32_t size;

  size = encodeChunk(c, sendbuf + pos + sizeof(size), BUFSIZE - pos);
  if (size <= 0) {
    fprintf(stderr,"output-chunkstream: Error encoding chunk or no space in output buffer, skipping\n");
  } else {
    *((uint32_t*)(sendbuf + pos)) = htonl(size);
    pos += sizeof(size) + size;
  }

  if (mode == TCP_MODE && fd < 0) {
    fprintf(stderr,"output-chunkstream: reconnecting ...\n");
    output_connect();
  }
  if (fd < 0) {
    fprintf(stderr,"output-chunkstream: not conected\n");
    return;
  }

  if (mode == TCP_MODE) {  //distiction needed by Win32
    ret = send(fd, sendbuf, pos, 0);
  } else {
    ret = write(fd, sendbuf, pos);
  }
  if (ret < 0) {
#ifndef _WIN32
    if (ret == -1 &&  (errno == EAGAIN || errno == EWOULDBLOCK)) {
#else
    if (ret == -1 &&  WSAGetLastError() == WSAEWOULDBLOCK) {
#endif
      fprintf(stderr,"output-chunkstream: Output stalled ...\n");
    } else {
      perror("output-chunkstream: Error writing to output");
      close(fd);
      pos = 0;
      fd = -1;
    }
  } else {
    dprintf("output-chunkstream: written %d bytes\n", ret);
    pos -= ret;
    memmove(sendbuf, sendbuf + ret, pos);
  }
}
