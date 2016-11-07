/*
 * Copyright (c) 2014-2015 Luca Baldesi
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
#include <stdlib.h>
#include <string.h>
//
#include <math.h>
#include <net_helper.h>
#include <peerset.h>
#include <peersampler.h>
#include <peer.h>
#include <grapes_msg_types.h>
//
#include "compatibility/timer.h"
//
#include "topology.h"
#include "streaming.h"
#include "dbg.h"
#include "measures.h"
#include "xlweighter.h"
#include "streamer.h"
#include "node_addr.h"

#define MAX(A,B) (((A) > (B)) ? (A) : (B))
#define NEIGHBOURHOOD_ADD 0
#define NEIGHBOURHOOD_REMOVE 1
#define DEFAULT_PEER_CBSIZE 50

#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif

double desired_bw = 0;	//TODO: turn on capacity measurement and set meaningful default value
double desired_rtt = 0.2;
double alpha_target = 0.4;
double topo_mem = 0.7;

bool topo_out = true; //peer selects out-neighbours
bool topo_in = true; //peer selects in-neighbours (combined means bidirectional)

bool topo_keep_best = false;
bool topo_add_best = false;

extern const char * xloptimization;

int NEIGHBOURHOOD_TARGET_SIZE = 30;
enum peer_choice {PEER_CHOICE_RANDOM, PEER_CHOICE_BEST, PEER_CHOICE_WORST};

struct metadata {
  uint16_t cb_size;
  uint16_t cps;
  float capacity;
  float recv_delay;
} __attribute__((packed));

struct topology_context{
	struct metadata my_metadata;	
	struct psample_context * tc;
	struct peerset * neighbourhood;
	struct peerset * swarm_bucket;
	struct peerset * locked_neighs;
	struct timeval tout_bmap;
	struct XLayerWeighter * xlw;
} context;

struct peerset * topology_get_neighbours()
{
	return context.neighbourhood;
}

void peerset_print(const struct peerset * pset,const char * name)
{
	const struct peer * p;
	int i;
	if(name) fprintf(stderr,"%s\n",name);
	if(pset)
		peerset_for_each(pset,p,i)
			fprintf(stderr,"\t%s\n",node_addr_tr(p->id));
}

void update_metadata()
{
	context.my_metadata.cb_size = am_i_source() ? 0 : get_cb_size();
	context.my_metadata.recv_delay = get_receive_delay();
	context.my_metadata.cps = get_chunks_per_sec();
	context.my_metadata.capacity = get_capacity();
}

struct peer * topology_get_peer(const struct nodeID * id)
{
	struct peer * p = NULL;
	p = peerset_get_peer(context.swarm_bucket,id);
	if(p == NULL)
		p = peerset_get_peer(context.neighbourhood,id);
	return p;
}

int topology_init(struct nodeID *myID,const char *config)
{
	bind_msg_type(MSG_TYPE_NEIGHBOURHOOD);
	bind_msg_type(MSG_TYPE_TOPOLOGY);
	update_metadata();
	context.tout_bmap.tv_sec = 20;
	context.tout_bmap.tv_usec = 0;
	context.tc = psample_init(myID,&(context.my_metadata),sizeof(struct metadata),config);

	context.neighbourhood = peerset_init(0);
	context.swarm_bucket = peerset_init(0);
  context.locked_neighs = peerset_init(0);

	if(xloptimization)
		context.xlw = xlweighter_new(xloptimization);
	else
		context.xlw = NULL;
  //fprintf(stderr,"[DEBUG] done with topology init\n");
	return context.tc && context.neighbourhood && context.swarm_bucket ? 1 : 0;
}

/*useful during bootstrap*/
int topology_node_insert(struct nodeID *id)
{
	struct metadata m = {0};
	if (topology_get_peer(id) == NULL)
		peerset_add_peer(context.swarm_bucket,id);
	return psample_add_peer(context.tc,id,&m,sizeof(m));
}

void topology_peer_set_metadata(struct  peer *p, const struct metadata *m)
{
	if (p)
	{
		if (m)
		{
			p->cb_size = m->cb_size;
			p->capacity = m->capacity;
		}
		else
		{
			p->cb_size = DEFAULT_PEER_CBSIZE;
			p->capacity = 0;
		}

	}
}

struct peer * neighbourhood_add_peer(const struct nodeID *id)
{
	struct peer * p = NULL;
	if (id)
	{
		p = peerset_pop_peer(context.swarm_bucket,id);
		if(p)
			peerset_push_peer(context.neighbourhood,p);
		else
		{
			peerset_add_peer(context.neighbourhood,id);
			p = peerset_get_peer(context.neighbourhood,id);
      peerset_push_peer(context.locked_neighs,p);
		}
		add_measures(p->id);
		send_bmap(id);
	}
	return p;
}

void neighbourhood_remove_peer(const struct nodeID *id)
{
	struct peer *p=NULL;
	if(id)
	{
		p = peerset_pop_peer(context.neighbourhood,id);
		if(p)
			peerset_push_peer(context.swarm_bucket,p);

    peerset_pop_peer(context.locked_neighs,id);
	}
}

void neighbourhood_message_parse(struct nodeID *from,const uint8_t *buff,int len)
{
	struct metadata m = {0};
	struct peer *p = NULL;

	switch(buff[0]) {
		case NEIGHBOURHOOD_ADD:
			//fprintf(stderr,"[DEBUG] adding peer %s from message\n",node_addr_tr(from));
			p = neighbourhood_add_peer(from);
			if (len >= (sizeof(struct metadata) + 1))
			{
				memmove(&m,buff+1,sizeof(struct metadata));
				topology_peer_set_metadata(p,&m);
			}
			break;

		case NEIGHBOURHOOD_REMOVE:
			neighbourhood_remove_peer(from);
			break;
		default:
			dprintf("Unknown neighbourhood message type");
	}
}

void topology_message_parse(struct nodeID *from, const uint8_t *buff, int len)
{
	switch(buff[0]) {
		case MSG_TYPE_NEIGHBOURHOOD:
			if (topo_in)
			{
				neighbourhood_message_parse(from,buff+1,len);
				reg_neigh_size(peerset_size(context.neighbourhood));
			}
			break;
		case MSG_TYPE_TOPOLOGY:
			psample_parse_data(context.tc,buff,len);
			//fprintf(stderr,"[DEBUG] received TOPO message\n");
			break;
		default:
			fprintf(stderr,"Unknown topology message type");
	}
}

void topology_sample_peers()
{
	int sample_nodes_num,sample_metas_num,i;
	const struct nodeID * const * sample_nodes;
	struct metadata const * sample_metas;
	struct peer * p;
		
	//fprintf(stderr,"[DEBUG] starting peer sampling\n");
	sample_nodes = psample_get_cache(context.tc,&sample_nodes_num);
	sample_metas = psample_get_metadata(context.tc,&sample_metas_num);
	for (i=0;i<sample_nodes_num;i++)
	{
		//fprintf(stderr,"[DEBUG] sampled node: %s\n",node_addr_tr(sample_nodes[i]));
		p = topology_get_peer(sample_nodes[i]);
		if(p==NULL)
		{
			//fprintf(stderr,"[DEBUG] NEW PEER!\n");
			peerset_add_peer(context.swarm_bucket,sample_nodes[i]);
			p = topology_get_peer(sample_nodes[i]);
		}
		else
			//fprintf(stderr,"[DEBUG] OLD PEER!\n");
		topology_peer_set_metadata(p,&(sample_metas[i]));	
	}
}

void topology_blacklist_add(struct nodeID * id)
{
}

void neighbourhood_drop_unactives(struct timeval * bmap_timeout)
{
  struct timeval tnow, told;
	struct peer *const *peers;
	int i;
  gettimeofday(&tnow, NULL);
  timersub(&tnow, bmap_timeout, &told);
  peers = peerset_get_peers(context.neighbourhood);
  for (i = 0; i < peerset_size(context.neighbourhood); i++) {
    if ( (!timerisset(&peers[i]->bmap_timestamp) && timercmp(&peers[i]->creation_timestamp, &told, <) ) ||
         ( timerisset(&peers[i]->bmap_timestamp) && timercmp(&peers[i]->bmap_timestamp, &told, <)     )   ) {
      dprintf("Topo: dropping inactive %s (peersset_size: %d)\n", node_addr_tr(peers[i]->id), peerset_size(context.neighbourhood));
      //if (peerset_size(context.neighbourhood) > 1) {	// avoid dropping our last link to the world
      topology_blacklist_add(peers[i]->id);
      neighbourhood_remove_peer(peers[i]->id);
      //}
    }
  }
	
}

void array_shuffle(void *base, int nmemb, int size) {
  int i,newpos;
  unsigned char t[size];
  unsigned char* b = base;

  for (i = nmemb - 1; i > 0; i--) {
    newpos = (rand()/(RAND_MAX + 1.0)) * (i + 1);
    memcpy(t, b + size * newpos, size);
    memmove(b + size * newpos, b + size * i, size);
    memcpy(b + size * i, t, size);
  }
}

//get the rtt. Currenly only MONL version is supported
static double get_rtt_of(const struct nodeID* n){
#ifdef MONL
  return get_rtt(n);
#else
  return NAN;
#endif
}

//get the declared capacity of a node
static double get_capacity_of(const struct nodeID* n){
  struct peer *p = topology_get_peer(n);
  if (p) {
    return p->capacity;
  }

  return NAN;
}

bool desiredness(const struct peer* p) {
	const struct nodeID* n = p->id;
  double rtt = get_rtt_of(n);
  double bw =  get_capacity_of(n);

  if ((isnan(rtt) && finite(desired_rtt)) || (isnan(bw) && desired_bw > 0)) {
    return false;
  } else if ((isnan(rtt) || rtt <= desired_rtt) && (isnan(bw) || bw >= desired_bw)) {
    return true;
  }

  return false;
}

int cmp_rtt(const void* p0, const void* p1) {
  double ra, rb;
	const struct nodeID * a = (*((struct peer * const*) p0)) -> id;
	const struct nodeID * b = (*((struct peer * const*) p1)) -> id;
  ra = get_rtt_of(a);
  rb = get_rtt_of(b);
  if ((isnan(ra) && isnan(rb)) || ra == rb) 
		return 0;
  else 
		if (isnan(rb) || ra < rb) 
			return -1;
  	else 
			return 1;
}

bool filter_xlweight(const struct peer *p)
{
	double w;

	w = xlweighter_peer_weight(context.xlw,p,get_my_addr());
	if(w>=0)
		return true;
	else
		return false;
}

int cmp_xlweight(const void* v0, const void* v1)
{
  double ra, rb;
  const struct peer * p0 = *(struct peer * const*)v0;
  const struct peer * p1 = *(struct peer * const*)v1;
	
	ra = xlweighter_peer_weight(context.xlw,p0,get_my_addr());
	rb = xlweighter_peer_weight(context.xlw,p1,get_my_addr());
  //fprintf(stderr,"[DEBUG] comparing %f and %f\n",ra,rb);

	if(ra < rb)
		return -1;
	if(ra > rb)
		return 1;
	return 0;
}

int neighbourhood_send_msg(const struct peer * p,uint8_t type)
{
	char * msg;
	int res;
	msg = malloc(sizeof(struct metadata)+2);
	msg[0] = MSG_TYPE_NEIGHBOURHOOD;
	msg[1] = type;
	memmove(msg+2,&(context.my_metadata),sizeof(struct metadata));
	res = send_to_peer(get_my_addr(),p->id,msg,sizeof(struct metadata)+2);
	return res;	
}

void peerset_destroy_reference_copy(struct peerset ** pset)
{
	while (peerset_size(*pset))
		peerset_pop_peer(*pset,(peerset_get_peers(*pset)[0])->id);

	peerset_destroy(pset);
}

struct peerset * peerset_create_reference_copy(const struct peerset * pset)
{
	struct peerset * ns;
	const struct peer * p;
	int i;

	ns = peerset_init(0);
	peerset_for_each(pset,p,i)
		peerset_push_peer(ns,p);
	return ns;
}

struct peer *nodeid_to_peer(const struct nodeID *id,int reg)
{
	struct peer * p;
	p = topology_get_peer(id);
	if(p==NULL && reg)
	{
		topology_node_insert(id);
		neighbourhood_add_peer(id);
		p = topology_get_peer(id);
		if(topo_out)
			neighbourhood_send_msg(p,NEIGHBOURHOOD_ADD);
	}
	return p;
}

/* move num peers from pset1 to pset2 after applying the filtering_mask function and following the given criterion */
void topology_move_peers(struct peerset * pset1, struct peerset * pset2,int num,enum peer_choice criterion,bool (*filter_mask)(const struct peer *),int (*cmp_peer)(const void* p0, const void* p1)) 
{
	struct peer * const * const_peers;
	struct peer ** peers;
	struct peer *p;
	int peers_num,i,j;

	peers_num = peerset_size(pset1);
	const_peers = peerset_get_peers(pset1);
	peers = (struct peer **)malloc(sizeof(struct peer *)*peers_num);
	if (filter_mask)
	{
		for(i = 0,j = 0; i<peers_num; i++)
			if (filter_mask(const_peers[i]))
				peers[j++] = const_peers[i];
		peers_num = j;
	} else
		memmove(peers,const_peers,peers_num*sizeof(struct peer*));

	if (criterion != PEER_CHOICE_RANDOM && cmp_peer != NULL) {
    //fprintf(stderr,"[DEBUG] choosen the qsort\n");
		qsort(peers, peers_num, sizeof(struct peer*), cmp_peer);
	} else {
    array_shuffle(peers, peers_num, sizeof(struct peer *));
	}
	for (i=0; i<peers_num && i<num; i++)
	{
		if (criterion == PEER_CHOICE_WORST)
			p = peerset_pop_peer(pset1,(peers[peers_num -i -1])->id);
		else
			p = peerset_pop_peer(pset1,(peers[i])->id);
		peerset_push_peer(pset2,p);
	}
}

void peerset_reference_copy_add(struct peerset * dst, struct peerset * src)
{
  const struct peer *p;
  int i;

	peerset_for_each(src,p,i)
		peerset_push_peer(dst,p);
}

void topology_signal_change(const struct peerset const * old_neighs)
{
	const struct peer * p;
	int i;
  reg_neigh_size(peerset_size(context.neighbourhood));

	// advertise changes
	if(topo_out)
	{
		peerset_for_each(context.neighbourhood,p,i)
    {
			if(peerset_check(old_neighs,p->id) < 0)
				neighbourhood_send_msg(p,NEIGHBOURHOOD_ADD);
    }
		peerset_for_each(old_neighs,p,i)
    {
			if(peerset_check(context.neighbourhood,p->id) < 0)
				neighbourhood_send_msg(p,NEIGHBOURHOOD_REMOVE);
    }
	}
}

void topology_update_xloptimization()
{
	int bests_num;
	int others_num;
  int i;
  const struct peer * p;

	topology_move_peers(context.neighbourhood,context.swarm_bucket,peerset_size(context.neighbourhood),PEER_CHOICE_RANDOM,NULL,NULL);
	xlweighter_base_nodes(context.xlw,context.swarm_bucket,get_my_addr());

  // keep locked peers in the neighbourhood
  peerset_for_each(context.locked_neighs,p,i)
  {
    peerset_pop_peer(context.swarm_bucket,p->id);
    peerset_push_peer(context.neighbourhood,p);
  }

  // try to fill the neighbourhood with xlweighted peers
	bests_num = MAX(NEIGHBOURHOOD_TARGET_SIZE-peerset_size(context.neighbourhood),0);
	topology_move_peers(context.swarm_bucket,context.neighbourhood,bests_num,PEER_CHOICE_BEST,filter_xlweight,cmp_xlweight);

  // filling with some other if room is available
	others_num = MAX(NEIGHBOURHOOD_TARGET_SIZE-peerset_size(context.neighbourhood),0);
	topology_move_peers(context.swarm_bucket,context.neighbourhood,others_num,PEER_CHOICE_RANDOM,NULL,NULL);
}

void topology_update_random()
{
	int discard_num;
	int others_num;

	discard_num = (int)((1-topo_mem) * peerset_size(context.neighbourhood));
	topology_move_peers(context.neighbourhood,context.swarm_bucket,discard_num,PEER_CHOICE_RANDOM,NULL,NULL);

	others_num = MAX(NEIGHBOURHOOD_TARGET_SIZE-peerset_size(context.neighbourhood),0);
	topology_move_peers(context.swarm_bucket,context.neighbourhood,others_num,PEER_CHOICE_RANDOM,NULL,NULL);
}

void topology_update_rtt()
{
	int discard_num;
	int bests_num;
	int others_num;

	// we keep topo_mem% of the current neighbourhood
	// so we discard the (1-topo_mem)% of the neighbourhood
	discard_num = (int)((1-topo_mem) * peerset_size(context.neighbourhood));
	topology_move_peers(context.neighbourhood,context.swarm_bucket,discard_num,PEER_CHOICE_WORST,NULL,cmp_rtt);

	// for the remaining 1-topo_mem fraction, we select (1-alpha_target)% of good nodes
	bests_num = (int)((1-alpha_target)*(MAX(NEIGHBOURHOOD_TARGET_SIZE-peerset_size(context.neighbourhood),0)));
	topology_move_peers(context.swarm_bucket,context.neighbourhood,bests_num,PEER_CHOICE_BEST,desiredness,cmp_rtt);

	// now we fill the neighbourhood with some other nodes (if any)
	others_num = MAX(NEIGHBOURHOOD_TARGET_SIZE-peerset_size(context.neighbourhood),0);
	topology_move_peers(context.swarm_bucket,context.neighbourhood,others_num,PEER_CHOICE_RANDOM,NULL,NULL);

}

void topology_update()
{
	struct peerset * old_neighs;
  const struct peer * p;
  int i;

  psample_parse_data(context.tc,NULL,0); // needed in order to trigger timed sending of TOPO messages

	update_metadata();
	topology_sample_peers();

  if timerisset(&(context.tout_bmap) )
		neighbourhood_drop_unactives(&(context.tout_bmap));

	old_neighs = peerset_create_reference_copy(context.neighbourhood);
  //fprintf(stderr,"[DEBUG] **********INIT**********\n");
  //peerset_print(old_neighs,"OLD_NEIGHS");
  //peerset_print(context.neighbourhood,"NEIGHS");
  //peerset_print(context.swarm_bucket,"SWARM_BUCKET");
  //peerset_print(context.locked_neighs,"LOCKED");

	if(xloptimization)
    topology_update_xloptimization();
	else
#ifdef MONL
    topology_update_rtt();
#else
    topology_update_random();
#endif

  topology_signal_change(old_neighs);
	peerset_destroy_reference_copy(&old_neighs);

	if(!xloptimization)
  {
    peerset_for_each(context.swarm_bucket,p,i)
      peerset_pop_peer(context.locked_neighs,p->id);
    peerset_clear(context.swarm_bucket,0);  // we don't remember past peers
  }
}
