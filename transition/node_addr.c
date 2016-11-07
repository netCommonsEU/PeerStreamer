#include <stdint.h>
#include <string.h>

#include <net_helper.h>

#include "node_addr.h"

const char * node_addr_tr(const struct nodeID *s) 
{
  static char addr[256];
  int ret;

  ret = node_addr(s, addr, 256);
  return ret >= 0 ? addr : NULL;
}
