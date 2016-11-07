#ifndef TOPMAN_H
#define TOPMAN_H

/** @file topmanager.h 
 *
 * @brief transitional file: conversion to peersampler.h
 *
 * This file serve to convert from the old TopologyManager interface to the new PeerSampler interface in GRAPES 0.2 and above
 * the use of this file is temporary, the new topology module will interface directly!
 * WARNING: the blacklist is not implemented
 *
*/

#include <peersampler.h>
struct psample_context;
static struct psample_context *tc;

inline const struct nodeID *const *topGetNeighbourhood(int *n)
{
  return psample_get_cache(tc, n);
}

inline const void *topGetMetadata(int *metadata_size)
{
  return psample_get_metadata(tc, metadata_size);
}

inline int topGrowNeighbourhood(int n)
{
  return psample_grow_cache(tc, n);
}

inline int topShrinkNeighbourhood(int n)
{
  return psample_shrink_cache(tc, n);
}

inline int topRemoveNeighbour(struct nodeID *neighbour)
{
  return psample_remove_peer(tc, neighbour);
}

inline int topChangeMetadata(void *metadata, int metadata_size)
{
  return psample_change_metadata(tc, metadata, metadata_size);
}

inline int topInit(struct nodeID *myID, void *metadata, int metadata_size, const char *config)
{
  tc = psample_init(myID, metadata, metadata_size, config);
  return tc ? 0 : -1;
}

inline int topAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
  return psample_add_peer(tc, neighbour, metadata, metadata_size);
}

inline int topParseData(const uint8_t *buff, int len)
{
 return psample_parse_data(tc, buff, len);
}

inline int topAddToBlackList(struct nodeID *neighbour)
{
  return 0;
}

inline int tmanAddToBlackList(struct nodeID *neighbour)
{
  return 0;
}

#endif //TOPMAN_H
