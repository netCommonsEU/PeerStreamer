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
#ifndef _WIN32
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#else
#include <winsock2.h>
#endif
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <net_helper.h>
#include <grapes_msg_types.h>
#include <peerset.h>
#include <peer.h>

#include "compatibility/timer.h"

#include "chunk_signaling.h"
#include "streaming.h"
#include "topology.h"
#include "loop.h"
#include "dbg.h"
#include "node_addr.h"

#define BUFFSIZE (512 * 1024)
#define FDSSIZE 16

extern bool neigh_log;

static int chunk_test_mtu;
static int chunk_test_socket;
static uint16_t chunk_test_port = 0;
struct sockaddr_storage chunk_test_dstudp = {0,};
uint32_t chunk_test_seqn = 0;

struct timeval period = {0, 500000};

//calculate timeout based on tnext
void tout_init(struct timeval *tout, const struct timeval *tnext)
{
  struct timeval tnow;

  gettimeofday(&tnow, NULL);
  if(timercmp(&tnow, tnext, <)) {
    timersub(tnext, &tnow, tout);
  } else {
    *tout = (struct timeval){0, 0};
  }
}

#define min(a, b)   (((a) > (b)) ? (b) : (a))
void chunk_test_forward(const uint8_t *buff, int len)
{
#define MON_DATA_HEADER_SPACE 32
#define MON_PKT_HEADER_SPACE  32
#define MSG_TYPE_CHUNK        0x11
  int pkt_len, offset;
  struct iovec iov[4];

  char h_pkt[MON_PKT_HEADER_SPACE];
  char h_data[MON_DATA_HEADER_SPACE];

#pragma pack(push)
#pragma pack(1)
  struct msg_header {
    uint32_t offset;
    uint32_t msg_length;
    int32_t local_con_id;
    int32_t remote_con_id;
    int32_t msg_seq_num;
    uint8_t msg_type;
    uint8_t len_mon_data_hdr;
    uint8_t len_mon_packet_hdr;
  } msg_h;
#define MSG_HEADER_SIZE (sizeof(struct msg_header))
#pragma pack(pop)

  iov[0].iov_base = &msg_h;
  iov[0].iov_len = MSG_HEADER_SIZE;

  msg_h.local_con_id = htonl(0);
  msg_h.remote_con_id = htonl(0);

  msg_h.msg_type = MSG_TYPE_CHUNK;
  msg_h.msg_seq_num = htonl(chunk_test_seqn++);

  iov[1].iov_len = iov[2].iov_len = 0;
  iov[1].iov_base = h_pkt;
  iov[2].iov_base = h_data;

  msg_h.len_mon_data_hdr = iov[2].iov_len;
  msg_h.len_mon_packet_hdr = iov[1].iov_len;

  offset = 0;

  do {
    int error, ret;
    struct msghdr msgh;

    pkt_len = min(chunk_test_mtu - iov[2].iov_len - iov[1].iov_len - iov[0].iov_len, len - offset) ;
    iov[3].iov_len = pkt_len;
    iov[3].iov_base = buff + offset;

    msg_h.offset = htonl(offset);
    msg_h.msg_length = htonl(len);

    msgh.msg_name = &chunk_test_dstudp;
    msgh.msg_namelen = sizeof(struct sockaddr_storage);
    msgh.msg_iov = iov;
    msgh.msg_iovlen = 4;
    msgh.msg_flags = 0;
    msgh.msg_control = NULL;
    msgh.msg_controllen = 0;

    ret = sendmsg(chunk_test_socket, &msgh, 0);

    if (ret  < 0){
      error = errno;
      fprintf(stderr, "Chunk test: sendmsg failed errno %d: %s\n", error, strerror(error));
      break;
    }

    offset += pkt_len;
  } while (offset != len);
}

void handle_msg(const struct nodeID* nodeid,bool source_role)
{
	uint8_t buff[BUFFSIZE];
	struct nodeID *remote;
	int len;

	len = recv_from_peer(nodeid, &remote, buff, BUFFSIZE);
	if (len < 0) {
		fprintf(stderr,"Error receiving message. Maybe larger than %d bytes\n", BUFFSIZE);
	}else
		switch (buff[0] /* Message Type */) {
			case MSG_TYPE_TMAN:
			case MSG_TYPE_NEIGHBOURHOOD:
			case MSG_TYPE_TOPOLOGY:
				dtprintf("Topo message received:\n");
				topology_message_parse(remote, buff, len);
				break;
			case MSG_TYPE_CHUNK:
				dtprintf("Chunk message received:\n");
				if(!source_role)
          if (chunk_test_port)
            chunk_test_forward(buff, len);
					received_chunk(remote, buff, len);
				break;
			case MSG_TYPE_SIGNALLING:
				dtprintf("Sign message received:\n");
				sigParseData(remote, buff, len);
				break;
			default:
				fprintf(stderr, "Unknown Message Type %x\n", buff[0]);
		}

	nodeid_free(remote);
}

void timeradd_inplace(struct timeval *base,struct timeval *toadd)
{
	struct timeval tmp;
	timeradd(base,toadd,&tmp);
	*base = tmp;
}

void usec2timeval(struct timeval *t, long usec)
{	
  t->tv_sec = usec / 1000000;
  t->tv_usec = usec % 1000000;
}

void loop_update(int loop_counter)
{
		if (loop_counter % 10 == 0)
			topology_update();
		if (neigh_log && loop_counter % 100 == 0)
		{
      log_neighbourhood();
#ifndef MONL
			log_nodes_measures();
#endif
		}
}

void spawn_chunk(int chunk_copies,struct timeval *chunk_time_interval)
{
  struct chunk *new_chunk;

	new_chunk = generated_chunk(&(chunk_time_interval->tv_usec));
	usec2timeval(chunk_time_interval,chunk_time_interval->tv_usec);
	if (new_chunk && add_chunk(new_chunk))
	{ 
		inject_chunk(new_chunk,chunk_copies);
		free(new_chunk); //if add_chunk fails it destroies the chunk
	}
}

void source_loop(const char *videofile, struct nodeID *nodeid, int csize, int chunk_copies, int buff_size)
/* source peer loop
 * @videofile: video input filename
 * @nodeid: local network identifier
 * @csize: chunks offer interval in microseconds
 * @chunk_copies: number of copies injected in the overlay
 * @buff_size: size of the chunk buffer
 */
{
	bool running=true;
	int data_ready,loop_counter=0;
	struct timeval awake_epoch, sleep_timer;
	struct timeval chunk_time_interval, offer_epoch, chunk_epoch, current_epoch; 
  int wait4fds[FDSSIZE],*pfds,fds[FDSSIZE] = {-1};

	usec2timeval(&period,csize);
	gettimeofday(&awake_epoch,NULL);
	timeradd(&awake_epoch,&period,&offer_epoch);
	chunk_epoch = awake_epoch;

  if (source_init(videofile, nodeid, fds, FDSSIZE, buff_size) < 0) {
    fprintf(stderr,"Cannot initialize source, exiting");
    exit(-1);
  }
	while(running)
	{
		if(fds[0] !=-1) {
			memmove(wait4fds,fds,sizeof(fds)); // bug in copy size?
			pfds = wait4fds;
		} else
			pfds = NULL;

  	tout_init(&sleep_timer, &awake_epoch);
		data_ready = wait4data(nodeid,&sleep_timer,pfds);

		switch(data_ready) {
			case 0: // timeout, no socket has data to pick
				gettimeofday(&current_epoch,NULL);
				if(timercmp(&offer_epoch, &current_epoch, <)) // offer time !
				{
					send_offer();
    			timeradd_inplace(&offer_epoch, &period);
				}
				else // chunk time ! <- needed for chunkisers withouth filedescritors, e.g., avf
				{
					spawn_chunk(chunk_copies,&chunk_time_interval);
					timeradd_inplace(&chunk_epoch, &chunk_time_interval); 
				}

				if(fds[0] == -1 && timercmp(&chunk_epoch, &offer_epoch, <)) // if we need a chunk timer
					awake_epoch = chunk_epoch;
				else
					awake_epoch = offer_epoch;
				break;

			case 1: //incoming msg
				handle_msg(nodeid,true);
				break;

			case 2: //file descriptor ready
				spawn_chunk(chunk_copies,&chunk_time_interval);
				break;
		
			default:
				fprintf(stderr,"[ERROR] select on file descriptors returned error: %d\n",data_ready);
		}
		loop_update(loop_counter++);
	}
}

void loop(struct nodeID *nodeid,int csize,int buff_size)
/* generic peer loop
 * @nodeid: local network identifier
 * @csize: chunks offer interval in microseconds
 * @buff_size: size of the chunk buffer
 */
{
	bool running=true;
	int data_ready,loop_counter=0;
	struct timeval epoch, wait_timer; 
	usec2timeval(&period,csize);
	gettimeofday(&epoch,NULL);

 	stream_init(buff_size, nodeid);
  topology_update();

	while(running)
	{
    tout_init(&wait_timer, &epoch);
    data_ready = wait4data(nodeid, &wait_timer, NULL);
		if(data_ready == 1)
			// we have been interrupted for an incoming msg
			handle_msg(nodeid,false);
		else
			// perform loop tasks
		{
			send_offer();
    	timeradd_inplace(&epoch, &period);
		}
		loop_update(loop_counter++);	
	}
}

int chunk_test_init(const uint16_t port, const char *ip, int mtu)
{
  int inet_status = 1;
  struct sockaddr_storage *tmp = &chunk_test_dstudp;

  chunk_test_mtu = mtu;
  chunk_test_port = port;

  tmp->ss_family = AF_INET;
  ((struct sockaddr_in *) tmp)->sin_port = htons(port);
  inet_status = inet_pton(tmp->ss_family, ip, &(((struct sockaddr_in *) tmp)->sin_addr));
  if (!inet_status) {
    fprintf(stderr, "Address family error in chunk test\n");
    return -1;
  }

  chunk_test_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (chunk_test_socket < 0) {
    fprintf(stderr, "Could not create chunk test socket\n");
    return -1;
  }

  fprintf(stdout, "Chunk test initialized: ip %s, port %d, mtu %d\n",
      ip, port, mtu);

  return 0;
}
