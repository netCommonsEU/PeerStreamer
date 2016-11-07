#ifndef __XLWEIGHTER_H__
#define __XLWEIGHTER_H__ 1

#include <stdint.h>
#include <peer.h>
#include <peerset.h>
#include <net_helper.h>

#define MAX_PATH_STRING_LENGTH 512

struct XLayerWeighter * xlweighter_new(const char * path_filename);

double xlweighter_peer_weight(const struct XLayerWeighter * xlw,const struct peer * p,const struct nodeID * me);

double xlweighter_base_nodes(struct XLayerWeighter * xlw,const struct peerset * pset,const struct nodeID * me);

void xlweighter_destroy(struct XLayerWeighter ** xlw);

#endif
