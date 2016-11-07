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
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif

#include <grapes_msg_types.h>
#include <net_helper.h>

#ifdef WIN32
#include <winsock2.h>
#include <unistd.h>
#endif

#include "net_helpers.h"
#include "loop.h"
#include "output.h"
#include "channel.h"
#include "topology.h"
#include "measures.h"
#include "streamer.h"
#include "node_addr.h"
#include "version.h"

#ifndef EXTRAVERSION
#define EXTRAVERSION "Unknown"
#endif

static struct nodeID *my_sock;

const char *peername = NULL;

static uint16_t chunk_test_port = 60006;
static const char *chunk_test_ip = "127.0.0.1";
static int chunk_test_mtu = 1372;

static const char *my_iface = NULL;
static int port = 6666;
static int srv_port;
static const char *srv_ip = "";
static int chunks_per_second = 25;
static double capacity_override = NAN;
static int multiply = 3;
int chunks_per_offer = 1;
static int buff_size = 50;
static int outbuff_size = 75;
static const char *fname = "/dev/stdin";
static const char *output_config ="";
const char * xloptimization = NULL;
static const char *net_helper_config = "";
static const char *topo_config = "";
unsigned char msgTypes[] = {MSG_TYPE_CHUNK,MSG_TYPE_SIGNALLING};
bool chunk_log = false;
bool signal_log = false;
bool neigh_log = false;
bool push_strategy = false;
unsigned int chunk_loss_interval = 0;
static int randomize_start = 0;
int start_id = -1;
int end_id = -1;
int initial_id = -1;

extern int NEIGHBOURHOOD_TARGET_SIZE;
extern uint64_t CB_SIZE_TIME;
extern double desired_bw;
extern double desired_rtt;
extern double alpha_target;
extern double topo_mem;
extern bool topo_out;
extern bool topo_in;
extern bool topo_keep_best;
extern bool topo_add_best;
extern bool autotune_period;
extern enum L3PROTOCOL {IPv4, IPv6} l3;

#ifndef MONL
extern struct timeval print_tdiff;
extern struct timeval tstartdiff;
#endif

static void print_usage(int argc, char *argv[])
{
  fprintf (stderr,
    "Usage:%s [options]\n"
    "\n"
    "Peer options\n"
    "\t[-p port]: port of the remote peer to connect at during bootstrap.\n"
    "\t           Usually it is the source peer port.\n"
    "\t[-i IP]: IP address of the remote peer to connect at during bootstrap.\n"
    "\t         Usually it is the source peer IP\n"
    "\t[-C name]: set the channel name to use on the repository.\n"
    "\t           All peers should use the same channel name.\n"
    "\n"
    "\t[-b size]: set the peer Chunk Buffer size.\n"
    "\t           This is also the chunk trading window size.\n"
    "\t[-o size]: set the Output Buffer size.\n"
    "\t[-c chunks]: set the number of chunks a peer can send per seconds.\n"
    "\t             it controls the upload capacity of peer as well.\n"
    "\t[-M peers]: neighbourhood target size.\n"
    "\t[-O chunks]: chunks sent per offer.\n"
    "\t[-t config]: topology config.\n"
    "\t[-P port]: local UDP port to be used by the peer.\n"
    "\t[-I iface]: local network interface to be used by the peer.\n"
    "\t         Useful if the host has several interfaces/addresses.\n"
    "\t[-6]: use IPv6 addressing space.\n"
    "\t[-N name]: set the name of the peer.\n"
    "\t         This name will be used when publishing in the repository.\n"
    "\t[-n options]: pass configuration options to the net-helper\n"
    "\t[--push_strategy]: use a loss-driven strategy for selecting peers in the source initial chunks push\n"
    "\t[--chunk_log]: print a chunk level log on stderr\n"
    "\t[--neighbourhood_log]: print neighbourhhod logs to stderr\n"
    "\t[--signal_log]: print signal logs on stderr\n"
    "\t[--chunk_loss_interval <interval>]: discard a chunk every <interval> number of chunks.\n"
    "\t[-F config]: configure the output module\n"
    "\t[-a alpha]: set the topology alpha value (from 0 to 100)\n"
    "\t[-r rtt]: set the RTT threshold (in ms) for desired neighbours\n"
    "\t[--desired_bw bw]: set the BW threshold (in bits/s) for desired neighbours. Use of K(ilo), M(ega) allowed, e.g 0.8M\n"
    "\t[--topo_mem p]: keep p (0..1) portion of peers between topology operations\n"
    "\t[--topo_out]: peers only choose out-neighbours\n"
    "\t[--topo_in]: peers only choose in-neighbours\n"
    "\t[--topo_bidir]: peers choose both in- and out-neighbours (bidir)\n"
    "\t[--topo_keep_best]: keep best peers, not random subset\n"
    "\t[--topo_add_best]: add best peers among desired ones, not random subset\n"
    "\t[--autotune_period]: automatically tune output bandwidth, 1:on, 0:off\n"
    "\t[--xloptimization]: pass a shortest-path file for cross layer optimization\n"
    "\n"
    "Special Source Peer options\n"
    "\t[-m chunks]: set the number of copies the source injects in the overlay.\n"
    "\t[-f filename]: name of the video stream file to transmit.\n"
    "\t[-S]: set initial chunk_id (source only).\n"
    "\t[-s]: set start_id from which to start output.\n"
    "\t[-e]: set end_id at which to end output.\n"
    "\n"
    "Special options\n"
    "\t[--randomize_start us]: random wait before starting [0..us] microseconds.\n"
    "\t[-v]: print version.\n"
    "\n"
#ifdef IO_GRAPES
    "NOTE: by default the peer will dump the received video on STDOUT in raw format\n"
    "      it can be played by your favourite player simply using a pipe\n"
    "      e.g., | cvlc /dev/stdin\n"
    "\n"
    "Examples:\n"
    "\n"
    "Start a source peer on port 6600:\n"
    "\n"
    "%s -f foreman.mpg -P 6600\n"
    "\n"
    "Start a peer connecting to the previous source, and using videolan as player:\n"
    "\n"
    "%s -i <sourceIP> -p <sourcePort> | cvlc /dev/stdin\n"
    "\n"
    "See usage examples on http://peerstreamer.org\n\n"
    "=======================================================\n", argv[0], argv[0], argv[0]
#else 
    "See usage examples on http://peerstreamer.org\n\n"
    "=======================================================\n", argv[0]
#endif
    );
  }


static double atod_kmg(const char *s) {
  double d;
  char *e;

  errno = 0;
  d = strtod(s, &e);
  if (errno) {
    fprintf(stderr, "Error parsing option: %s\n", s);
    exit(-1);
  }
  switch (*e) {
    case 'g':
    case 'G':
      d *= 1024;
    case 'm':
    case 'M':
      d *= 1024;
    case 'k':
    case 'K':
      d *= 1024;
    case 0:
      break;
    default:
      fprintf(stderr, "Error parsing option: %s\n", s);
      exit(-1);
  }

  return d;
}

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  int option_index = 0;
  static struct option long_options[] = {
        {"signal_log", no_argument, 0, 0},
        {"chunk_log", no_argument, 0, 0},
        {"neighbourhood_log", no_argument, 0, 0},
        {"push_strategy", no_argument, 0, 0},
        {"chunk_loss_interval", required_argument, 0, 0},
        {"measure_start", required_argument, 0, 0},
        {"measure_every", required_argument, 0, 0},
        {"playout_limit", required_argument, 0, 0},
        {"randomize_start", required_argument, 0, 0},
        {"capacity_override", required_argument, 0, 0},
        {"desired_bw", required_argument, 0, 0},
        {"topo_mem", required_argument, 0, 0},
        {"topo_in", no_argument, 0, 0},
        {"topo_out", no_argument, 0, 0},
        {"topo_bidir", no_argument, 0, 0},
        {"topo_keep_best", no_argument, 0, 0},
        {"topo_add_best", no_argument, 0, 0},
        {"autotune_period", required_argument, 0, 0},
        {"xloptimization", required_argument, 0, 0},
	{0, 0, 0, 0}
  };

    while ((o = getopt_long (argc, argv, "r:a:b:o:O:c:p:i:P:I:f:F:m:lC:N:n:M:t:s:e:S:6v:x:y:z:",long_options, &option_index)) != -1) { //use this function to manage long options
    switch(o) {
      case 0: //for long options
        if( strcmp( "chunk_log", long_options[option_index].name ) == 0 ) { chunk_log = true; }
        if( strcmp( "neighbourhood_log", long_options[option_index].name ) == 0 ) { neigh_log = true; }
        if( strcmp( "signal_log", long_options[option_index].name ) == 0 ) { signal_log = true; }
        if( strcmp( "push_strategy", long_options[option_index].name ) == 0 ) { push_strategy = true; }
        if( strcmp( "chunk_loss_interval", long_options[option_index].name ) == 0 ) { chunk_loss_interval = atoi(optarg); }
#ifndef MONL
        if( strcmp( "measure_start", long_options[option_index].name ) == 0 ) { tstartdiff.tv_sec = atoi(optarg); }
        if( strcmp( "measure_every", long_options[option_index].name ) == 0 ) { print_tdiff.tv_sec = atoi(optarg); }
#endif
        if( strcmp( "playout_limit", long_options[option_index].name ) == 0 ) { CB_SIZE_TIME = atoi(optarg); }
        if( strcmp( "randomize_start", long_options[option_index].name ) == 0 ) { randomize_start = atoi(optarg); }
        if( strcmp( "capacity_override", long_options[option_index].name ) == 0 ) { capacity_override = atod_kmg(optarg); }
        if( strcmp( "desired_bw", long_options[option_index].name ) == 0 ) { desired_bw = atod_kmg(optarg); }
        if( strcmp( "topo_mem", long_options[option_index].name ) == 0 ) { topo_mem = atof(optarg); }
        else if( strcmp( "topo_in", long_options[option_index].name ) == 0 ) { topo_in = true; topo_out = false; }
        else if( strcmp( "topo_out", long_options[option_index].name ) == 0 ) { topo_in = false; topo_out = true; }
        else if( strcmp( "topo_bidir", long_options[option_index].name ) == 0 ) { topo_in = true; topo_out = true; }
        else if( strcmp( "topo_keep_best", long_options[option_index].name ) == 0 ) { topo_keep_best = true; }
        else if( strcmp( "topo_add_best", long_options[option_index].name ) == 0 ) { topo_add_best = true; }
        else if( strcmp( "autotune_period", long_options[option_index].name ) == 0 ) { autotune_period = (bool) atoi(optarg); }
        else if( strcmp( "xloptimization", long_options[option_index].name ) == 0 ) { xloptimization = strdup((const char *) optarg); }
        break;
      case 'a':
        alpha_target = (double)atoi(optarg) / 100.0;
        break;
      case 'r':
        desired_rtt = (double)atoi(optarg) / 1000.0;
        break;
      case 'b':
        buff_size = atoi(optarg);
        break;
      case 'o':
        outbuff_size = atoi(optarg);
        break;
      case 'c':
        chunks_per_second = atoi(optarg);
        break;
      case 'm':
        multiply = atoi(optarg);
        break;
      case 'O':
        chunks_per_offer = atoi(optarg);
        break;
      case 'p':
        srv_port = atoi(optarg);
        break;
      case 'i':
        srv_ip = strdup(optarg);
        break;
      case 'P':
        port =  atoi(optarg);
        break;
      case 'I':
        my_iface = strdup(optarg);
        break;
      case '6':
    	l3 = IPv6;
    	break;
      case 'f':
        fname = strdup(optarg);
        break;
      case 'F':
        output_config = strdup(optarg);
        break;
      case 'C':
        channel_set_name(optarg);
        break;
      case 'n':
        net_helper_config = strdup(optarg);
        break;
      case 'N':
        peername = strdup(optarg);
        break;
      case 'M':
        NEIGHBOURHOOD_TARGET_SIZE = atoi(optarg);
        break;
      case 't':
        topo_config = strdup(optarg);
        break;
      case 'S':
        initial_id = atoi(optarg);
        break;
      case 's':
        start_id = atoi(optarg);
        break;
      case 'e':
        end_id = atoi(optarg);
        break;
      case 'v':
        fprintf(stderr, "Version: %s\n", EXTRAVERSION);
        fprintf(stderr, "\tGRAPES: %s\n", GRAPES_VERSION);
        fprintf(stderr, "\tNAPA-BASELIBS: %s\n", NAPA_BASELIBS_VERSION);
        fprintf(stderr, "\tffmpeg: %s\n", FFMPEG_VERSION);
        fprintf(stderr, "\tlibogg: %s\n", LIBOGG_VERSION);
        fprintf(stderr, "\tlibvorbis: %s\n", LIBVORBIS_VERSION);
        fprintf(stderr, "\tmp3lame: %s\n", MP3LAME_VERSION);
        fprintf(stderr, "\tx264: %s\n", X264_VERSION);
        fprintf(stderr, "\tlibconfuse: %s\n", LIBCONFUSE_VERSION);
        fprintf(stderr, "\tlibevent: %s\n", LIBEVENT_VERSION);
        fprintf(stderr, "\tlibxml2: %s\n", LIBXML2_VERSION);
	    exit(0);
      case 'x':
        chunk_test_port = atoi(optarg);
        break;
      case 'y':
        chunk_test_ip = strdup(optarg);
        break;
      case 'z':
        chunk_test_mtu = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);
        print_usage(argc, argv);

        exit(-1);
    }
  }

  if (!channel_get_name()) {
    channel_set_name("generic");
  }

  if (argc <= 1) {
    print_usage(argc, argv);
    fprintf(stderr, "Trying to start a source with default parameters, reading from %s\n", fname);
  }
}

const struct nodeID *get_my_addr(void)
{
  return my_sock;
}

static void init_rand(const char * s)
{
  long i, l, x;
  struct timeval t;

  gettimeofday(&t, NULL);

  x = 1;
  l = strlen(s);
  for (i = 0; i < l; i++) {
    x *= s[i];
  }

  srand(t.tv_usec * t.tv_sec * x);
}

static struct nodeID *init(void)
{
  int i = 0;
  struct nodeID *myID = NULL;
  char *my_addr = NULL;
  FILE* fp = NULL;

#ifdef _WIN32
  {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", err);
        return NULL;
    }
  }
#endif

  if (my_iface) {
    my_addr = iface_addr(my_iface);
  } else {
    my_addr = default_ip_addr();
  }

  if (my_addr == NULL) {
    fprintf(stderr, "Cannot find network interface %s\n", my_iface);

    return NULL;
  }
  for (i=0;i<2;i++)
	  bind_msg_type(msgTypes[i]);
  myID = net_helper_init(my_addr, port, net_helper_config);
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);
    free(my_addr);

    return NULL;
  }
  free(my_addr);
  fprintf(stderr, "My network ID is: %s\n", node_addr_tr(myID));

  // For communication with the GUI
  // chunker_player needs to know the network ID in order
  // to publish it in the repo
  fp = fopen("NetworkID","w");
  if(fp)
  {
    fprintf(fp,"%s\n",node_addr_tr(myID));
    fprintf(fp,"IDEnd");
    fclose(fp);
  }

  init_rand(node_addr_tr(myID));

  topology_init(myID, topo_config);

  return myID;
}

void leave(int sig) {
  fprintf(stderr, "Received signal %d, exiting!\n", sig);
  end_measures();
  exit(sig);
}

// wait [0..max] microsec
static void random_wait(int max) {
    uint64_t us;
#ifndef _WIN32
    struct timespec t;

    us = (rand()/(RAND_MAX + 1.0)) * max;
    t.tv_sec = us / 1000000;
    t.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&t, NULL);
#else
    us = (rand()/(RAND_MAX + 1.0)) * max;
    //Sleep(us / 1000000);
    usleep(us % 1000000);
#endif
}

int main(int argc, char *argv[])
{

  (void) signal(SIGTERM,leave);
  (void) signal(SIGINT,leave);

  cmdline_parse(argc, argv);

  my_sock = init();
  if (my_sock == NULL) {
    fprintf(stderr, "Cannot initialize streamer, exiting!\n");
    return -1;
  }
  if (srv_port != 0) {
    fprintf(stderr, "Hi, I play the generic peer role\n");
    struct nodeID *srv;

    if (chunk_test_init(chunk_test_port, chunk_test_ip, chunk_test_mtu)) {
      fprintf(stderr, "Cannot initialize chunk test: %s:%d\n", chunk_test_ip, chunk_test_port);
      return -1;
    }

    //random wait a bit before starting
    if (randomize_start) random_wait(randomize_start);

    output_init(outbuff_size, output_config);

    srv = create_node(srv_ip, srv_port);
	
    if (srv == NULL) {
      fprintf(stderr, "Cannot resolve remote address %s:%d\n", srv_ip, srv_port);

      return -1;
    }
    topology_node_insert(srv);

    loop(my_sock, 1000000 / chunks_per_second, buff_size);
  } else {
    fprintf(stderr, "Hi, I play the source role\n");
    source_loop(fname, my_sock, 1000000 / chunks_per_second, multiply, buff_size);
  }
  return 0;
}

int am_i_source()
{
  return (srv_port == 0);
}

int get_cb_size()
{
  return buff_size;
}

int get_chunks_per_sec()
{
  return chunks_per_second;
}

//capacity in bits/s, or NAN if unknown
double get_capacity()
{
  return capacity_override;
}
