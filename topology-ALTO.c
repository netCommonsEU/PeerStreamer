/*
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2010-2011 Luca Abeni
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
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h> /* qsort, rand */

#include <net_helper.h>
#include <peerset.h>
#include <peer.h>
#include <grapes_msg_types.h>
#include <topmanager.h>
#include <ALTOclient.h>
#include <pthread.h>

#include "net_helpers.h"	/* default_ip_addr() */
#include "topology.h"
#include "streaming.h"
#include "dbg.h"
#include "measures.h"
#include "config-ALTO.h"
#include "node_addr.h"

static int NEIGHBORHOOD_TARGET_SIZE;

static struct peerset *pset;
static struct timeval tout_bmap = {10, 0};

#define LOG_EVERY	1000
static int cnt = 0;

static struct nodeID *me;
static unsigned char mTypes[] = {MSG_TYPE_TOPOLOGY};
static uint64_t currtime;
static struct nodeID **altoList;
static int altoList_size, c_neigh_size, newAltoResults=2;
static struct nodeID **currentNeighborhood;

/* ALTO begin --> */
#define ALTO_MAX_PEERS	1024
static int ALTO_bucket_size, RAND_bucket_size;

/* work struct */
static struct tagALTOInfo {
  struct in_addr localIPAddr;
  ALTO_GUIDANCE_T peers[ALTO_MAX_PEERS];
} ALTOInfo;

/* input to the peer selection function */
typedef struct {
  const struct nodeID **neighbours;
  int numNeighb;
  unsigned int pri_crit;	/* primary rating criterion */
  unsigned int sec_crit;	/* secondary rating criterion flags */
  float altoFactor;			/* percentage of ALTO selected peers in disjoint bucket */
  /* jahanpanah@neclab.eu */
} ALTOInput_t;
/* <-- ALTO end */

void add_peer(struct nodeID *id)
{
      dprintf("Adding %s to neighbourhood!\n", node_addr_tr(id));
      peerset_add_peer(pset, id);
      /* add measures here */
      add_measures(id);
      send_bmap(id);
}

void remove_peer(struct nodeID *id)
{
      dprintf("Removing %s from neighbourhood!\n", node_addr_tr(id));
      /* add measures here */
      delete_measures(id);
      peerset_remove_peer(pset, id);
}

/* ALTO begin --> */

/**
 * As nodeID is an opaque type not exposing its in_addr, we need to convert the
 * IP string returned by node_addr_tr() via inet_aton().
 */
static void node_convert_addr(const struct nodeID* node, struct in_addr* addr) {
  int rescode;

//  const char* peerAddr = node_addr_tr(node);

  /* extract IP string without port */
  const char *peerIP = node_ip(node);

//  fprintf(stderr,"peer addr = %s IP = %s\n", peerAddr, peerIP);

  /* fill in IP addr */
  rescode = inet_aton(peerIP, addr);
  assert(rescode != 0);
}

/*
  for a given address find corresponding entry in unsorted
  neighbour list provided by streamer
*/
static const struct nodeID* findALTOPeerInNeighbourList(const struct nodeID **neighbours, int numNeighb, size_t altoEntry)
{
  int p; struct nodeID *res;
  struct in_addr addr;

  for (p=0; p < numNeighb; p++) {
    if (!neighbours[p]) continue;
    node_convert_addr(neighbours[p], &addr);
    if (addr.s_addr == ALTOInfo.peers[altoEntry].alto_host.s_addr) {
      /* we found the entry */
		res = neighbours[p];
		neighbours[p] = NULL;
      return res;
	}
  }
  return NULL; /* not found */
}

/** qsort comparator function for ALTO peer rankings */
static int qsort_compare_ALTO_ranking(const void* a, const void* b)
{
  const ALTO_GUIDANCE_T* r1 = (const ALTO_GUIDANCE_T*) a;
  const ALTO_GUIDANCE_T* r2 = (const ALTO_GUIDANCE_T*) b;
  return r1->rating - r2->rating;
}

static void topo_ALTO_init(char* myIP)
{
	NEIGHBORHOOD_TARGET_SIZE = g_config.neighborhood_target_size;
	dprintf("myIP: %s\n", myIP);
	assert(inet_aton(myIP, &ALTOInfo.localIPAddr) != 0);
	start_ALTO_client();
	/*set_ALTO_server("http://10.10.251.107/cgi-bin/alto-server.cgi");
	  set_ALTO_server("http://www.napa-wine-alto.eu/cgi-bin/alto-server.cgi");*/
	fprintf(stderr,"Setting ALTO server URL: '%s'\n", g_config.alto_server);
	set_ALTO_server(g_config.alto_server);

	srand(time(NULL));
}

void topologyShutdown(void)
{
  if (g_config.alto_server) stop_ALTO_client();
  config_free();
}

/*
  The main peer selector function.
*/
void PeerSelectorALTO(void)
{
	int p, i, rescode, j = 0;
	struct timeval tnow;
	uint64_t timenow;

	gettimeofday(&tnow, NULL);
	timenow = (tnow.tv_usec + tnow.tv_sec * 1000000ull);
	if ((timenow - currtime > (g_config.update_interval * 1000000)) &&
			(newAltoResults)) {

		/* fill the temporary peer list (used for ALTO query) */
		for (p=0; p < c_neigh_size; p++) {
			/* copy/convert nodeID IP address into ALTO struct */
			node_convert_addr(currentNeighborhood[p], &ALTOInfo.peers[p].alto_host);

			ALTOInfo.peers[p].prefix = 32;
			ALTOInfo.peers[p].rating = -1; /* init to dummy rating */
		}

		/*****************************************************************/
		/*   ALTO query                                                  */
		/*****************************************************************/
		gettimeofday(&tnow, NULL);
		timenow = (tnow.tv_usec + tnow.tv_sec * 1000000ull);
		fprintf(stderr,"Calling ALTO server at : %u\n",(unsigned int)(((unsigned long)timenow)/1000000)); //
		rescode = ALTO_query_exec( // get_ALTO_guidance_for_list(
				ALTOInfo.peers,
				c_neigh_size,
				ALTOInfo.localIPAddr,
				g_config.alto_pri_criterion,
				g_config.alto_sec_criterion
		);

		/*assert(rescode == 1);*/
		if (rescode != 1) {
			fprintf(stderr,"WARNING: ALTO query FAILED!\n");
			newAltoResults = 1;
		}
		else {
			newAltoResults = 0;
		}
	}

	if (ALTO_query_state() == ALTO_QUERY_READY) {

		if (!newAltoResults) {
			gettimeofday(&tnow, NULL);
			currtime = (tnow.tv_usec + tnow.tv_sec * 1000000ull);
			fprintf(stderr,"Received ALTO server reply at : %u\n",(unsigned int)(((unsigned long)currtime)/1000000));

			/* Build sorted list of ALTO-rated peers */
			qsort(ALTOInfo.peers, c_neigh_size, sizeof(ALTO_GUIDANCE_T), qsort_compare_ALTO_ranking);

#ifdef LOG_ALTO_RATINGS
			dprintf("\nSorted ALTO ratings:\n");
			for (p=0; p < c_neigh_size; p++) {
				dprintf("ALTO peer %d: rating = %d\n ", p, ALTOInfo.peers[p].rating);
			}
#endif

			/*
    now build the disjoint buckets of peers:

    we pick a certain percentage of the best ALTO-rated peers
    and fill the rest with random neighbours
			 */

			for (i=0; i < altoList_size; i++) {
				nodeid_free(altoList[i]);
			}

			if ((NEIGHBORHOOD_TARGET_SIZE > 0) && (NEIGHBORHOOD_TARGET_SIZE < c_neigh_size)) {
				altoList_size = NEIGHBORHOOD_TARGET_SIZE;
			} else {
				altoList_size = c_neigh_size;
			}
			ALTO_bucket_size = altoList_size * g_config.alto_factor;
			RAND_bucket_size = altoList_size - ALTO_bucket_size;

			altoList = realloc(altoList,altoList_size*sizeof(struct nodeID *));
			altoList = memset(altoList,0,altoList_size*sizeof(struct nodeID *));

			/* add ALTO peers */
			fprintf(stderr,"\nSorted ALTO peers:\n");
			for (i = 0; i < ALTO_bucket_size; i++) {
				altoList[j] = findALTOPeerInNeighbourList(currentNeighborhood, c_neigh_size, i);
				if (altoList[j]) {
					fprintf(stderr,"ALTO peer %d: id  = %s ; rating = %d\n ", (j+1), node_addr_tr(altoList[j]), ALTOInfo.peers[i].rating);
					j++;
				}
			}

		/* add remaining peers randomly */
		fprintf(stderr,"\nMore ALTO randomly picked peers:\n");
		for (i = 0; i < RAND_bucket_size; i++) {
			do { // FIXME: it works only if gossipNeighborhood is realloc'ed for sure between two queries...
				p = rand() % c_neigh_size;
			} while (!currentNeighborhood[p]);

			altoList[j] = currentNeighborhood[p];
			currentNeighborhood[p] = NULL;
			fprintf(stderr,"ALTO peer %d: id  = %s\n ", (j+1), node_addr_tr(altoList[j]));
			j++;
		}
		newAltoResults = 1;
		altoList_size = j;
	}
	}

}

/* <-- ALTO end */

static void topoAddToBL (struct nodeID *id)
{
	topAddToBlackList(id);
}

int topoAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
	return topAddNeighbour(neighbour,metadata,metadata_size);
}

int topologyInit(struct nodeID *myID, const char *config)
{
	me = myID;
	bind_msg_type(mTypes[0]);
	config_init();
	config_load("streamer.conf");
	if (g_config.alto_server && (strcmp(g_config.alto_server, "") == 0)) g_config.alto_server = NULL;	//handle the case of empty string
	//config_dump();
	if (g_config.alto_server) topo_ALTO_init(node_ip(myID));
	return (topInit(myID, NULL, 0, config));
}


void update_peers(struct nodeID *from, const uint8_t *buff, int len)
{
	int i,j,k,npeers,doAdd,psize;
	struct peer *peers; static const struct nodeID **nbrs;
	struct timeval told,tnow;

	psize = peerset_size(pset);
	dprintf("before:%d, ",psize);
	topParseData(buff, len);
	nbrs = topGetNeighbourhood(&npeers);
	if (newAltoResults) {
		for (i=0; i < c_neigh_size; i++) {
			nodeid_free(currentNeighborhood[i]);
		}
		free(currentNeighborhood);
		c_neigh_size = npeers + psize;
		currentNeighborhood = c_neigh_size ? calloc(c_neigh_size,sizeof(struct nodeID *)) : NULL;
		for (i=0; i < psize; i++) {
			currentNeighborhood[i] = nodeid_dup((peerset_get_peers(pset)[i]).id);
		}
		for (j=0; j < npeers; j++) {
			if (peerset_check(pset, nbrs[j]) < 0) {
				currentNeighborhood[i] = nodeid_dup(nbrs[j]);
				i++;
			}
		}
		c_neigh_size = i;
		currentNeighborhood = i?realloc(currentNeighborhood,c_neigh_size * sizeof(struct nodeID *)):NULL;
	}
	if (g_config.alto_server && c_neigh_size > 1) {
		PeerSelectorALTO();
	}

	if (g_config.alto_server && ALTO_query_state() == ALTO_QUERY_READY && newAltoResults==1) {
	//	fprintf(stderr,"Composing peerset from ALTO selection\n");
		for (i=0; i < altoList_size; i++) { /* first goal : add all new ALTO-ranked peers
											second goal : do not increase current peerset size
											third goal : make space by deleting only peers that
												are not in the current ALTO-reply */
			if (peerset_check(pset, altoList[i]) < 0) {
				doAdd = 1;
				if (NEIGHBORHOOD_TARGET_SIZE &&
					peerset_size(pset) >= NEIGHBORHOOD_TARGET_SIZE) {
					doAdd = 0;
					for (j = 0; j < peerset_size(pset); j++) {
						for (k = 0; k < altoList_size; k++) {
							if (k!=i && nodeid_equal(peerset_get_peers(pset)[j].id, altoList[k])) {
								break;
							}
						}
						if (k == altoList_size) {
							remove_peer(peerset_get_peers(pset)[j].id);
							doAdd = 1;
							break; // without this, peerset size would possibly decrease
						}
					}
				}
				if (doAdd) {
					add_peer(altoList[i]);
				}
			}
		}
		newAltoResults = 2;
	}
	if (!NEIGHBORHOOD_TARGET_SIZE || peerset_size(pset) < NEIGHBORHOOD_TARGET_SIZE) {
		// ALTO selection didn't fill the peerset
		//	fprintf(stderr,"Composing peerset from ncast neighborhood\n");
		for (i=0;i<npeers;i++) {
			if(peerset_check(pset, nbrs[i]) < 0) {
				add_peer(nbrs[i]);
			}
		}
	}
	psize = peerset_size(pset);
	dprintf("after:%d, ",psize);

	gettimeofday(&tnow, NULL);
	timersub(&tnow, &tout_bmap, &told);
	peers = peerset_get_peers(pset);

	if (LOG_EVERY && ++cnt % LOG_EVERY == 0) {
		fprintf(stderr,"\nMy peerset : size = %d\n",psize);
		for (i=0;i<psize;i++) {
			fprintf(stderr,"\t%d : %s\n",i,node_addr_tr(peers[i].id));
		}
	}

	for (i = 0; i < peerset_size(pset); i++) {
		if ( (!timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].creation_timestamp, &told, <) ) ||
				( timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].bmap_timestamp, &told, <)     )   ) {
			//if (peerset_size(pset) > 1) {	// avoid dropping our last link to the world
			topoAddToBL(peers[i].id);
			remove_peer(peers[i--].id);
			//}
		}
	}

	reg_neigh_size(peerset_size(pset));

	dprintf("after timer check:%d\n",peerset_size(pset));
}

struct peer *nodeid_to_peer(const struct nodeID* id, int reg)
{
	struct peer *p = peerset_get_peer(pset, id);
	if (!p) {
		//fprintf(stderr,"warning: received message from unknown peer: %s!\n",node_addr_tr(id));
		if (reg && peerset_size(pset) < NEIGHBORHOOD_TARGET_SIZE) {
	//      topoAddNeighbour(id, NULL, 0);	//@TODO: this is agressive
			add_peer(id);
			p = peerset_get_peer(pset,id);
		}
	}

	return p;
}

int peers_init()
{
	fprintf(stderr,"peers_init\n");
	pset = peerset_init(0);
	return pset ? 1 : 0;
}

struct peerset *get_peers()
{
	return pset;
}
