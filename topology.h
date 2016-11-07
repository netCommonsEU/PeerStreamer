/*
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2010-2011 Luca Abeni
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
#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <stdint.h>

#define MSG_TYPE_NEIGHBOURHOOD   0x22

struct peerset *topology_get_neighbours(void);
void topology_update();
struct peer *nodeid_to_peer(const struct nodeID* id, int reg);
int topology_node_insert(struct nodeID *neighbour);
int topology_init(struct nodeID *myID, const char *config);
void topology_message_parse(struct nodeID *from, const uint8_t *buff, int len);
void peerset_print(const struct peerset * pset,const char * name);

#endif	/* TOPOLOGY_H */
