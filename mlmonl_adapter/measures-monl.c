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
#include <math.h>
#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY       (1.0/0.0)
#endif

#include <mon.h>
#include <ml.h>
#include <net_helper.h>
#include <grapes_msg_types.h>

#include "../channel.h"
#include "../dbg.h"
#include "measures-monl.h"
#include "node_addr.h"

#define PEER_PUBLISH_INTERVAL 10 //in seconds
#define P2P_PUBLISH_INTERVAL 60 //in seconds

extern const char *peername;

typedef struct nodeID {
	socketID_handle addr;
	int connID;	// connection associated to this node, -1 if myself
	int refcnt;
	//a quick and dirty static vector for measures TODO: make it dinamic
	MonHandler mhs[20];
	int n_mhs;
} nodeID;

static MonHandler chunk_dup = -1, chunk_playout = -1 , neigh_size = -1, chunk_receive = -1, chunk_send = -1, offer_accept_in = -1, offer_accept_out = -1, chunk_hops = -1, chunk_delay = -1, playout_delay = -1;
static MonHandler queue_delay = -1 , offers_in_flight = -1;
static MonHandler period = -1;

//static MonHandler rx_bytes_chunk_per_sec, tx_bytes_chunk_per_sec, rx_bytes_sig_per_sec, tx_bytes_sig_per_sec;
//static MonHandler rx_chunks, tx_chunks;

/*
 * Start one measure
*/
void start_measure(MonHandler mh, MonParameterValue rate, const char *pubname, enum stat_types st[], int length, SocketId dst, MsgType mt)
{
	if (rate) monSetParameter (mh, P_PUBLISHING_RATE, rate);
	if (length) monPublishStatisticalType(mh, pubname, channel_get_name(), st , length, NULL);
	monActivateMeasure(mh, dst, mt);
}

/*
 * Initialize and start one measure
 */
void add_measure(MonHandler *mhp, MeasurementId id, MeasurementCapabilities mc, MonParameterValue rate, const char *pubname, enum stat_types st[], int length, SocketId dst, MsgType mt)
{
	*mhp = monCreateMeasure(id, mc);
	start_measure(*mhp, rate, pubname, st, length, dst, mt);
}

/*
 * Register duplicate arrival
*/
void reg_chunk_duplicate()
{
	if (chunk_dup < 0) {
		enum stat_types st[] = {SUM, RATE};
		// number of chunks which have been received more then once
		add_measure(&chunk_dup, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ChunkDuplicates", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks]
		monNewSample(chunk_dup, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_dup, 1);
}

/*
 * Register playout/loss of a chunk before playout
*/
void reg_chunk_playout(int id, bool b, uint64_t timestamp)
{
	static MonHandler chunk_loss_burst_size;
	static int last_arrived_chunk = -1;

	struct timeval tnow;
	if (chunk_playout < 0 && b) {	//don't count losses before the first arrived chunk
		enum stat_types st[] = {WIN_AVG, AVG, SUM, RATE};
		//number of chunks played
		add_measure(&chunk_playout, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ChunksPlayed", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks]
	}
	monNewSample(chunk_playout, b);

	if (playout_delay < 0) {
		enum stat_types st[] = {WIN_AVG, WIN_VAR};
		//delay after reorder buffer, however chunkstream module does not use reorder buffer
		add_measure(&playout_delay, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ReorderDelay", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[seconds]
	}
	if (b) {	//count delay only if chunk has arrived
		gettimeofday(&tnow, NULL);
		monNewSample(playout_delay, ((int64_t)(tnow.tv_usec + tnow.tv_sec * 1000000ULL) - (int64_t)timestamp) / 1000000.0);
	}

	//if (!chunk_loss_burst_size) {
	//	enum stat_types st[] = {WIN_AVG, WIN_VAR};
	//	// number of consecutive lost chunks
	//	add_measure(&chunk_loss_burst_size, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ChunkLossBurstSize", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks]
	//}
	if (b) {
		if (last_arrived_chunk >= 0) {
			int burst_size = id - 1 - last_arrived_chunk;
			if (burst_size) monNewSample(chunk_loss_burst_size, burst_size);
		}
		last_arrived_chunk = id;
	}
}

/*
 * Register actual neghbourhood size
*/
void reg_neigh_size(int s)
{
	if (neigh_size < 0) {
		enum stat_types st[] = {LAST, WIN_AVG};
		// number of peers in the neighboorhood
		add_measure(&neigh_size, GENERIC, 0, PEER_PUBLISH_INTERVAL, "NeighSize", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
	}
	monNewSample(neigh_size, s);
}

/*
 * Register chunk receive event
*/
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount, bool old, bool dup)
{
	struct timeval tnow;

	if (chunk_receive < 0) {
		enum stat_types st[] = {RATE};
		// total number of received chunks per second
		add_measure(&chunk_receive, GENERIC, 0, PEER_PUBLISH_INTERVAL, "TotalRxChunk", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks/s]
		monNewSample(chunk_receive, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_receive, 1);

	if (chunk_hops < 0) {
		enum stat_types st[] = {WIN_AVG};
		// number of hops from source on the p2p network
		add_measure(&chunk_hops, GENERIC, 0, PEER_PUBLISH_INTERVAL, "OverlayHops", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
	}
	monNewSample(chunk_hops, hopcount);

	if (chunk_delay < 0) {
		enum stat_types st[] = {WIN_AVG, WIN_VAR};
		// time elapsed since the source emitted the chunk
		add_measure(&chunk_delay, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ReceiveDelay", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[seconds]
	}
	gettimeofday(&tnow, NULL);
	monNewSample(chunk_delay, ((int64_t)(tnow.tv_usec + tnow.tv_sec * 1000000ULL) - (int64_t)timestamp) / 1000000.0);
}

/*
 * Register chunk send event
*/
void reg_chunk_send(int id)
{
	if (chunk_send < 0) {
		enum stat_types st[] = {RATE};
		add_measure(&chunk_send, GENERIC, 0, PEER_PUBLISH_INTERVAL, "TotalTxChunk", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks/s]
		monNewSample(chunk_send, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_send, 1);
}

/*
 * Register chunk accept event
*/
void reg_offer_accept_in(bool b)
{
	if (offer_accept_in < 0) {
		enum stat_types st[] = {WIN_AVG};
		// ratio between number of offers and number of accepts
		add_measure(&offer_accept_in, GENERIC, 0, PEER_PUBLISH_INTERVAL, "OfferAcceptIn", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[no unit -> ratio]
	}
	monNewSample(offer_accept_in, b);
}

/*
 * Register chunk accept event
*/
void reg_offer_accept_out(bool b)
{
	if (offer_accept_out < 0) {
		enum stat_types st[] = {WIN_AVG};
		// ratio between number of offers and number of accepts
		add_measure(&offer_accept_out, GENERIC, 0, PEER_PUBLISH_INTERVAL, "OfferAcceptOut", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[no unit -> ratio]
	}
	monNewSample(offer_accept_out, b);
}


/*
 * Register the number of offers in flight at each offer sent event
*/
void reg_offers_in_flight(int running_offer_threads)
{
	if (offers_in_flight < 0) {
		enum stat_types st[] =  {AVG, WIN_AVG, LAST};
		add_measure(&offers_in_flight, GENERIC, 0, PEER_PUBLISH_INTERVAL, "OffersInFlight", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
		monNewSample(offers_in_flight, 0);	//force publish even if there are no events
	}
	else {
		monNewSample(offers_in_flight, running_offer_threads);
	}
}

/*
 * Register queue delay at each ack received event
*/
void reg_queue_delay(double last_queue_delay)
{
	if (queue_delay < 0) {
		enum stat_types st[] =  {AVG, WIN_AVG, LAST};
		add_measure(&queue_delay, GENERIC, 0, PEER_PUBLISH_INTERVAL, "QueueDelay", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
		monNewSample(queue_delay, 0);	//force publish even if there are no events
	}
	monNewSample(queue_delay, last_queue_delay);
}

/*
 * Register period time at each change
*/
void reg_period(double last_period)
{
	if (period < 0) {
		enum stat_types st[] =  {WIN_AVG};
		add_measure(&period, GENERIC, 0, PEER_PUBLISH_INTERVAL, "Period", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
		monNewSample(period, 0);	//force publish even if there are no events
	}
	monNewSample(period, last_period/1000000);
}

/*
 * Initialize peer level measurements
*/
void init_measures()
{
	if (peername) monSetPeerName(peername);
}

/*
 * End peer level measurements
*/
void end_measures()
{
}

/*
 * Initialize p2p measurements towards a peer
*/
void add_measures(struct nodeID *id)
{
	// Add measures
	int j = 0;
	enum stat_types stwinavgwinvar[] = {WIN_AVG, WIN_VAR};
	enum stat_types stwinavg[] = {WIN_AVG};
//      enum stat_types stwinavgrate[] = {WIN_AVG, RATE};
//	enum stat_types stsum[] = {SUM};
	enum stat_types stsumwinsumrate[] = {SUM, PERIOD_SUM, WIN_SUM, RATE};

	dprintf("adding measures to %s\n",node_addr_tr(id));

	/* HopCount */
	// number of hops at IP level
       id->mhs[j] = monCreateMeasure(HOPCOUNT, PACKET | IN_BAND);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "HopCount", stwinavg, sizeof(stwinavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[IP hops]

	/* Round Trip Time */
       id->mhs[j] = monCreateMeasure(RTT, PACKET | IN_BAND);
       monSetParameter (id->mhs[j], P_WINDOW_SIZE, 30);	//make RTT measurement more reactive. Default sliding window is 100 packets
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "RoundTripDelay", stwinavgwinvar, sizeof(stwinavgwinvar)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[seconds]

	/* Loss */
       id->mhs[j] = monCreateMeasure(SEQWIN, PACKET | IN_BAND);
       start_measure(id->mhs[j++], 0, NULL, NULL, 0, id->addr, MSG_TYPE_CHUNK);
       id->mhs[j] = monCreateMeasure(LOSS, PACKET | IN_BAND | REMOTE_RESULTS);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "LossRate", stwinavg, sizeof(stwinavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//LossRate_avg [probability 0..1] LossRate_rate [lost_pkts/sec]

       /* RX,TX volume in bytes (only chunks) */
       id->mhs[j] = monCreateMeasure(RX_BYTE, PACKET | IN_BAND);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "RxBytesChunk", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes]
       id->mhs[j] = monCreateMeasure(TX_BYTE, PACKET | IN_BAND);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "TxBytesChunk", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes]

       /* RX,TX volume in bytes (only signaling) */
       id->mhs[j] = monCreateMeasure(RX_BYTE, PACKET | IN_BAND);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "RxBytesSig", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[bytes]
       id->mhs[j] = monCreateMeasure(TX_BYTE, PACKET | IN_BAND);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "TxBytesSig", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[bytes]

	// Chunks
       id->mhs[j] = monCreateMeasure(RX_PACKET, DATA | IN_BAND);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "RxChunks", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//RxChunks_sum [chunks] RxChunks_rate [chunks/sec]
       id->mhs[j] = monCreateMeasure(TX_PACKET, DATA | IN_BAND);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "TxChunks", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//TxChunks_sum [chunks] TxChunks_rate [chunks/sec]

/*
//	// Capacity
	id->mhs[j] = monCreateMeasure(CLOCKDRIFT, PACKET | IN_BAND);
	monSetParameter (id->mhs[j], P_CLOCKDRIFT_ALGORITHM, 1);
	start_measure(id->mhs[j++], 0, NULL, NULL, 0, id->addr, MSG_TYPE_CHUNK);
	id->mhs[j] = monCreateMeasure(CORRECTED_DELAY, PACKET | IN_BAND);
	start_measure(id->mhs[j++], 0, NULL, NULL, 0, id->addr, MSG_TYPE_CHUNK);
	id->mhs[j] = monCreateMeasure(CAPACITY_CAPPROBE, PACKET | IN_BAND);
	monSetParameter (id->mhs[j], P_CAPPROBE_DELAY_TH, -1);
//	monSetParameter (id->mhs[j], P_CAPPROBE_PKT_TH, 5);
	start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "Capacity", stwinavg, sizeof(stwinavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes/s]

	//Available capacity
       id->mhs[j] = monCreateMeasure(BULK_TRANSFER, PACKET | DATA | IN_BAND);
       start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "BulkTransfer", stwinavg, sizeof(stwinavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK); //Bulktransfer [bit/s]
	id->mhs[j] = monCreateMeasure(AVAILABLE_BW_FORECASTER, PACKET | IN_BAND);
	start_measure(id->mhs[j++], P2P_PUBLISH_INTERVAL, "AvailableBW", stwinavg, sizeof(stwinavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes/s]
*/

	// for static must not be more then 10 or whatever size is in net_helper-ml.c
	id->n_mhs = j;
}

/*
 * Delete p2p measurements towards a peer
*/
void delete_measures(struct nodeID *id)
{
	dprintf("deleting measures from %s\n",node_addr_tr(id));
	while(id->n_mhs) {
		monDestroyMeasure(id->mhs[--(id->n_mhs)]);
	}
}

/*
 * Helper to retrieve a measure
*/
double get_measure(struct nodeID *id, int j, enum stat_types st)
{
	return (id->n_mhs > j) ? monRetrieveResult(id->mhs[j], st) : NAN;
}

/*
 * Hopcount to a given peer
*/
int get_hopcount(struct nodeID *id){
	double r = get_measure(id, 0, LAST);
	return isnan(r) ? -1 : (int) r;
}

/*
 * RTT to a given peer in seconds
*/
double get_rtt(struct nodeID *id){
	return get_measure(id, 1, WIN_AVG);
}

/*
 * average RTT to a set of peers in seconds
*/
double get_average_rtt(struct nodeID **ids, int len){
	int i;
	int n = 0;
	double sum = 0;

	for (i = 0; i < len; i++) {
		double l = get_rtt(ids[i]);
		if (!isnan(l)) {
			sum += l;
			n++;
		}
	}
	return (n > 0) ? sum / n : NAN;
}

/*
 * loss ratio from a given peer as 0..1
*/
double get_lossrate(struct nodeID *id){
	return get_measure(id, 3, WIN_AVG);
}

/*
 * loss ratio from a given peer as 0..1
*/
double get_transmitter_lossrate(struct nodeID *id){
	return (monRetrieveResultById(id->addr, MSG_TYPE_CHUNK, PACKET | IN_BAND | REMOTE, LOSS, WIN_AVG));
}

/*
 * average loss ratio from a set of peers as 0..1
*/
double get_average_lossrate(struct nodeID **ids, int len){
	int i;
	int n = 0;
	double sum = 0;

	for (i = 0; i < len; i++) {
		double l = get_lossrate(ids[i]);
		if (!isnan(l)) {
			sum += l;
			n++;
		}
	}
	return (n > 0) ? sum / n : NAN;
}

double get_receive_delay(void) {
	return chunk_delay >= 0 ? monRetrieveResult(chunk_delay, WIN_AVG) : NAN;
}
