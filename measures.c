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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY       (1.0/0.0)
#endif
#include <sys/time.h>

#include "compatibility/timer.h"

#include "measures.h"
#include "grapes_msg_types.h"
#include "streamer.h"
#include "node_addr.h"
#include "list.h"

struct timeval print_tdiff = {3600, 0};
struct timeval tstartdiff = {60, 0};
static struct timeval tstart;
static struct timeval print_tstart;
static struct timeval tnext;

struct measures {
  int duplicates;
  int chunks;
  int played;
  int64_t sum_reorder_delay;

  int chunks_received_dup, chunks_received_nodup, chunks_received_old;
  int sum_hopcount;
  int64_t sum_receive_delay;

  int chunks_sent;

  uint64_t sum_neighsize;
  int samples_neighsize;

  uint64_t bytes_sent, bytes_sent_chunk, bytes_sent_sign, bytes_sent_topo;
  int msgs_sent, msgs_sent_chunk, msgs_sent_sign, msgs_sent_topo;

  uint64_t bytes_recvd, bytes_recvd_chunk, bytes_recvd_sign, bytes_recvd_topo;
  int msgs_recvd, msgs_recvd_chunk, msgs_recvd_sign, msgs_recvd_topo;

  uint64_t sum_offers_in_flight;
  int samples_offers_in_flight;
  double sum_queue_delay;
  int samples_queue_delay;
  double sum_period;
  int samples_period;

  int offers_out;
  int accepts_out;
  int offers_in;
  int accepts_in;
};

static struct list_head node_stats; // list of node statistics
static struct measures m;

void clean_measures()
{
  memset(&m, 0, sizeof(m));
}

double tdiff_sec(const struct timeval *a, const struct timeval *b)
{
  struct timeval tdiff;
  timersub(a, b, &tdiff);
  return tdiff.tv_sec + tdiff.tv_usec / 1000000.0;
}

void print_measure(const char *name, double value)
{
  static const struct nodeID *my_addr;
  static char *my_addr_str;

  //cache address to avoid recalculating it at every call
  if (my_addr != get_my_addr()) {
    if (my_addr) nodeid_free(my_addr);
    my_addr = nodeid_dup(get_my_addr());
    my_addr_str = strdup(node_addr_tr(my_addr));
  }

  fprintf(stderr,"abouttopublish,%s,,,%s,%f,,,%f\n", my_addr_str, name, value, tdiff_sec(&tnext, &tstart));
}

void print_measures()
{
  struct timeval tnow;
  double timespan;

  gettimeofday(&tnow, NULL);
  timespan = tdiff_sec(&tnow, &print_tstart);

  if (m.chunks) print_measure("PlayoutRatio", (double)m.played / m.chunks);
  if (m.chunks) print_measure("ReorderDelay(ok&lost)", (double)m.sum_reorder_delay / 1e6 / m.chunks);
  if (m.samples_neighsize) print_measure("NeighSize", (double)m.sum_neighsize / m.samples_neighsize);
  if (m.chunks_received_nodup) print_measure("OverlayDistance(intime&nodup)", (double)m.sum_hopcount / m.chunks_received_nodup);
  if (m.chunks_received_nodup) print_measure("ReceiveDelay(intime&nodup)", (double)m.sum_receive_delay / 1e6 / m.chunks_received_nodup);

  if (timerisset(&print_tstart)) print_measure("ChunkRate", (double) m.chunks / timespan);
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(all)", (double) (m.chunks_received_old + m.chunks_received_nodup + m.chunks_received_dup)  / timespan);
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(old)", (double) m.chunks_received_old / timespan);
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(intime&nodup)", (double) m.chunks_received_nodup / timespan);
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(intime&dup)", (double) m.chunks_received_dup / timespan);
  if (timerisset(&print_tstart)) print_measure("ChunkSendRate", (double) m.chunks_sent / timespan);

  if (timerisset(&print_tstart)) {
    print_measure("SendRateMsgs(all)", (double) m.msgs_sent / timespan);
    print_measure("SendRateMsgs(chunk)", (double) m.msgs_sent_chunk / timespan);
    print_measure("SendRateMsgs(sign)", (double) m.msgs_sent_sign / timespan);
    print_measure("SendRateMsgs(topo)", (double) m.msgs_sent_topo / timespan);
    print_measure("SendRateMsgs(other)", (double) (m.msgs_sent - m.msgs_sent_chunk - m.msgs_sent_sign - m.msgs_sent_topo) / timespan);

    print_measure("SendRateBytes(all)", (double) m.bytes_sent / timespan);
    print_measure("SendRateBytes(chunk)", (double) m.bytes_sent_chunk / timespan);
    print_measure("SendRateBytes(sign)", (double) m.bytes_sent_sign / timespan);
    print_measure("SendRateBytes(topo)", (double) m.bytes_sent_topo / timespan);
    print_measure("SendRateBytes(other)", (double) (m.bytes_sent - m.bytes_sent_chunk - m.bytes_sent_sign - m.bytes_sent_topo) / timespan);

    print_measure("RecvRateMsgs(all)", (double) m.msgs_recvd / timespan);
    print_measure("RecvRateMsgs(chunk)", (double) m.msgs_recvd_chunk / timespan);
    print_measure("RecvRateMsgs(sign)", (double) m.msgs_recvd_sign / timespan);
    print_measure("RecvRateMsgs(topo)", (double) m.msgs_recvd_topo / timespan);
    print_measure("RecvRateMsgs(other)", (double) (m.msgs_recvd - m.msgs_recvd_chunk - m.msgs_recvd_sign - m.msgs_recvd_topo) / timespan);

    print_measure("RecvRateBytes(all)", (double) m.bytes_recvd / timespan);
    print_measure("RecvRateBytes(chunk)", (double) m.bytes_recvd_chunk / timespan);
    print_measure("RecvRateBytes(sign)", (double) m.bytes_recvd_sign / timespan);
    print_measure("RecvRateBytes(topo)", (double) m.bytes_recvd_topo / timespan);
    print_measure("RecvRateBytes(other)", (double) (m.bytes_recvd - m.bytes_recvd_chunk - m.bytes_recvd_sign - m.bytes_recvd_topo) / timespan);
  }

  if (m.chunks_received_old + m.chunks_received_nodup + m.chunks_received_dup) print_measure("ReceiveRatio(intime&nodup-vs-all)", (double)m.chunks_received_nodup / (m.chunks_received_old + m.chunks_received_nodup + m.chunks_received_dup));

  if (m.samples_offers_in_flight) print_measure("OffersInFlight", (double)m.sum_offers_in_flight / m.samples_offers_in_flight);
  if (m.samples_queue_delay) print_measure("QueueDelay", m.sum_queue_delay / m.samples_queue_delay);
  if (m.samples_period) print_measure("Period", m.sum_period / m.samples_period);

  if (timerisset(&print_tstart)) {
    print_measure("OfferOutRate", (double) m.offers_out / timespan);
    print_measure("AcceptOutRate", (double) m.accepts_out / timespan);
  }
  if (m.offers_out) print_measure("OfferAcceptOutRatio", (double)m.accepts_out / m.offers_out);

  if (timerisset(&print_tstart)) {
    print_measure("OfferInRate", (double) m.offers_in / timespan);
    print_measure("AcceptInRate", (double) m.accepts_in / timespan);
  }
  if (m.offers_in) print_measure("OfferAcceptInRatio", (double)m.accepts_in / m.offers_in);
}

bool print_every()
{
  static bool startup = true;
  struct timeval tnow;

  gettimeofday(&tnow, NULL);
  if (startup) {
    if (!timerisset(&tstart)) {
      timeradd(&tnow, &tstartdiff, &tstart);
      print_tstart = tstart;
    }
    if (timercmp(&tnow, &tstart, <)) {
      return false;
    } else {
      startup = false;
    }
  }

  if (!timerisset(&tnext)) {
    timeradd(&tstart, &print_tdiff, &tnext);
  }
  if (!timercmp(&tnow, &tnext, <)) {
    print_measures();
    clean_measures();
    print_tstart = tnext;
    timeradd(&tnext, &print_tdiff, &tnext);
  }
  return true;
}

/*
 * Register duplicate arrival
*/
void reg_chunk_duplicate()
{
  if (!print_every()) return;

  m.duplicates++;
}

/*
 * Register playout/loss of a chunk before playout
*/
void reg_chunk_playout(int id, bool b, uint64_t timestamp)
{
  struct timeval tnow;

  if (!print_every()) return;

  m.played += b ? 1 : 0;
  m.chunks++;
  gettimeofday(&tnow, NULL);
  m.sum_reorder_delay += (tnow.tv_usec + tnow.tv_sec * 1000000ULL) - timestamp;
}

/*
 * Register actual neghbourhood size
*/
void reg_neigh_size(int s)
{
  if (!print_every()) return;

  m.sum_neighsize += s;
  m.samples_neighsize++;
}

/*
 * Register chunk receive event
*/
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount, bool old, bool dup)
{
  struct timeval tnow;

  if (!print_every()) return;

  if (old) {
    m.chunks_received_old++;
  } else {
    if (dup) { //duplicate detection works only for in-time arrival
      m.chunks_received_dup++;
    } else {
      m.chunks_received_nodup++;
      m.sum_hopcount += hopcount;
      gettimeofday(&tnow, NULL);
      m.sum_receive_delay += (tnow.tv_usec + tnow.tv_sec * 1000000ULL) - timestamp;
    }
  }
}

/*
 * Register chunk send event
*/
void reg_chunk_send(int id)
{
  if (!print_every()) return;

  m.chunks_sent++;
}

/*
 * Register offer-accept transaction initited by us (accept receive event)
*/
void reg_offer_accept_out(bool b)
{
  if (!print_every()) return;

  m.offers_out++;
  if (b) m.accepts_out++;
}

/*
 * Register offer-accept transaction initited by others (offer receive event)
*/
void reg_offer_accept_in(bool b)
{
  if (!print_every()) return;

  m.offers_in++;
  if (b) m.accepts_in++;
}

/*
 * messages sent (bytes vounted at message content level)
*/
void reg_message_send(int size, uint8_t type)
{
  if (!print_every()) return;

  m.bytes_sent += size;
  m.msgs_sent++;

  switch (type) {
   case MSG_TYPE_CHUNK:
     m.bytes_sent_chunk+= size;
     m.msgs_sent_chunk++;
     break;
   case MSG_TYPE_SIGNALLING:
     m.bytes_sent_sign+= size;
     m.msgs_sent_sign++;
     break;
   case MSG_TYPE_TOPOLOGY:
   case MSG_TYPE_TMAN:
     m.bytes_sent_topo+= size;
     m.msgs_sent_topo++;
     break;
   default:
     break;
  }
}

/*
 * messages sent (bytes vounted at message content level)
*/
void reg_message_recv(int size, uint8_t type)
{
  if (!print_every()) return;

  m.bytes_recvd += size;
  m.msgs_recvd++;

  switch (type) {
   case MSG_TYPE_CHUNK:
     m.bytes_recvd_chunk+= size;
     m.msgs_recvd_chunk++;
     break;
   case MSG_TYPE_SIGNALLING:
     m.bytes_recvd_sign+= size;
     m.msgs_recvd_sign++;
     break;
   case MSG_TYPE_TOPOLOGY:
   case MSG_TYPE_TMAN:
     m.bytes_recvd_topo+= size;
     m.msgs_recvd_topo++;
     break;
   default:
     break;
  }
}

/*
 * Register the number of offers in flight
*/
void reg_offers_in_flight(int running_offers_threads)
{
  if (!print_every()) return;

  m.sum_offers_in_flight += running_offers_threads;
  m.samples_offers_in_flight++;
}

/*
 * Register the sample for RTT
*/
void reg_queue_delay(double last_queue_delay)
{
  if (!print_every()) return;

  m.sum_queue_delay += last_queue_delay;
  m.samples_queue_delay++;
}

/*
 * Register the offer period
*/
void reg_period(double period)
{
  if (!print_every()) return;

  m.sum_period += period;
  m.samples_period++;
}

/*
 * Initialize peer level measurements
*/
void init_measures()
{
	INIT_LIST_HEAD(&node_stats);	
}

/*
 * End peer level measurements
*/
void end_measures()
{
  print_measures();
}

struct node_statistics * get_node_statistics(const struct nodeID * id)
{
	struct node_statistics * ns = NULL;
	struct list_head * ptr;
	bool found = false;
	list_for_each_conditioned(ptr,&node_stats,found){
		ns = list_entry(ptr,struct node_statistics,list);
		if (nodeid_cmp(ns->node,  id) == 0) 
			found = true; // to stop the loop
	}
	return found?ns:NULL;
}

double get_reception_rate_measure(const struct nodeID *id)
{
	struct node_statistics * ns = get_node_statistics(id);
	if(ns)
		return ns->reception_rate;
	else
		return 0;
}

/*
 * Initialize p2p measurements towards a peer
*/
void add_measures(const struct nodeID *id)
{
	if (get_node_statistics(id) == NULL) {
		struct node_statistics* node_stat = (struct node_statistics*)malloc(sizeof(struct node_statistics));
		node_stat->node = nodeid_dup(id);
		node_stat->reception_rate = 1-MIN_RATE_VALUE;
		node_stat->offer_accept_rtt = -1;
		list_add(&(node_stat->list),&node_stats);
	}
}

/*
 * Delete p2p measurements towards a peer
*/
void delete_measures(const struct nodeID *id)
{
	struct list_head * ptr,*swap;
	struct node_statistics * elem;
	list_for_each_safe(ptr,swap,&node_stats){
		elem = list_entry(ptr,struct node_statistics,list);
		if (nodeid_cmp(elem->node,id)) {
			list_del(ptr);
			free(elem->node);
			free(elem);
		}
	}
}

void timeout_reception_measure(const struct nodeID *id)
{
	struct node_statistics * ns;
	ns = get_node_statistics(id);
	if (ns != NULL) {
		ns->reception_rate = (ns->reception_rate)*(1-(SAMPLE_WEIGHT));
		if(ns->reception_rate < MIN_RATE_VALUE)
			ns->reception_rate = MIN_RATE_VALUE;
	}
}

void reception_measure(const struct nodeID *id)
{
	struct node_statistics * ns;
	ns = get_node_statistics(id);
	if (ns != NULL) {
		ns->reception_rate = (SAMPLE_WEIGHT) + (ns->reception_rate)*(1-(SAMPLE_WEIGHT));
		if(ns->reception_rate > 1-MIN_RATE_VALUE)
			ns->reception_rate = 1-MIN_RATE_VALUE;
	}
}

void offer_accept_rtt_measure(const struct nodeID *id,const uint64_t oa_rtt)
{
	struct node_statistics * ns;
	ns = get_node_statistics(id);
	if (ns != NULL) {
		if (ns->offer_accept_rtt >= 0)
			ns->offer_accept_rtt = oa_rtt * (SAMPLE_WEIGHT) + (ns->offer_accept_rtt)*(1-(SAMPLE_WEIGHT));
		else
			ns->offer_accept_rtt = oa_rtt;
	}
}

void log_nodes_measures()
/*print to stderr: [STAT_LOG],node_address,reception_rate,offer_accept_rtt*/
{
	struct node_statistics * ptr;
	char str[NODE_STR_LENGTH]; 
	list_for_each_entry(ptr,&node_stats,list) {
		node_addr(ptr->node,str,NODE_STR_LENGTH);
		fprintf(stderr,"[STAT_LOG],%s,%f,%f\n",str, 
				ptr->reception_rate,ptr->offer_accept_rtt);
	}
}

double get_receive_delay(void) {
	return m.chunks_received_nodup ? (double)m.sum_receive_delay / 1e6 / m.chunks_received_nodup : NAN;
}
