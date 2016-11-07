/*
 * Copyright (c) 2010-2011 Stefano Traverso
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

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdbool.h>

/* timeout of the offers thread. If it is not updated, it is deleted */
#define TRANS_ID_MAX_LIFETIME 10.0

struct nodeID;

// register the moment when a transaction is started
// return a  new transaction id
uint16_t transaction_create(struct nodeID *id);

// Add the moment I received a positive select in a list
// return true if a valid trans_id is found
bool transaction_reg_accept(uint16_t trans_id,const struct nodeID *id);

// Used to get the time elapsed from the moment I get a positive select to the moment i get the ACK
// related to the same chunk
// it return -1.0 in case no trans_id is found
double transaction_remove(uint16_t trans_id);

#endif // TRANSACTION_H
