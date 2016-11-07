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
#ifndef MEASURES_H
#define MEASURES_H

#include <stdbool.h>
#ifndef MONL
#include "list.h"
#include "malloc.h"
#endif

struct nodeID;

// alpha weight for the first-order mobile mean update
#define SAMPLE_WEIGHT 0.01

#ifndef MONL
#define MIN_RATE_VALUE 0.1

struct node_statistics {
	struct list_head list;
	struct nodeID * node;

	double reception_rate; // ratio of expected msg arrived
	double offer_accept_rtt;
};
void offer_accept_rtt_measure(const struct nodeID *id,const uint64_t oa_rtt);
void reception_measure(const struct nodeID *id);
void timeout_reception_measure(const struct nodeID *id);
void log_nodes_measures();
double get_reception_rate_measure(const struct nodeID *id);
#endif

void init_measures();
void end_measures();
void add_measures(struct nodeID *id);
void delete_measures(struct nodeID *id);

void reg_chunk_duplicate();
void reg_chunk_playout(int id, bool b, uint64_t timestamp);
void reg_neigh_size(int s);
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount, bool old, bool dup);
void reg_chunk_send(int id);
void reg_offer_accept_out(bool b);
void reg_offer_accept_in(bool b);
void reg_offers_in_flight(int running_offer_threads);
void reg_queue_delay(double last_queue_delay);
void reg_period(double period);

double get_receive_delay(void);
#ifdef MONL
double get_rtt(struct nodeID *id);
double get_rtt_var(struct nodeID *id);
double get_lossrate(struct nodeID *id);
double get_transmitter_lossrate(struct nodeID *id);
double get_average_lossrate(struct  nodeID**id, int len);
int get_hopcount(struct nodeID *id);
#endif

#endif
