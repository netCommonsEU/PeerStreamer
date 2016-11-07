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

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dbg.h"
#include "measures.h"
#include "transaction.h"

typedef struct {
	uint16_t trans_id;
	double offer_sent_time;
	double accept_received_time;
	struct nodeID *id;
	} service_time;

// List to trace peer's service times
struct service_times_element {
	service_time st;
	struct service_times_element *backward;
	struct service_times_element *forward;
	};

static struct service_times_element *stl = NULL;

// Check the service times list to find elements over the timeout
void check_neighbor_status_list() {
	struct service_times_element *stl_iterator, *stl_aux;
	struct timeval current_time;
	bool something_got_removed;

	gettimeofday(&current_time, NULL);
	something_got_removed = false;
	
        dprintf("LIST: check trans_id list\n");
	
	// Check if list is empty
	if (stl == NULL) {
		return;
		}
        
	// Start from the beginning of the list
	stl_iterator = stl;
	stl_aux = stl;
	// Iterate the list until you get the right element
	while (stl_iterator != NULL) {
		// If the element has been in the list for a period greater than the timeout, remove it
//		if ( (stl_iterator->st.accept_received_time > 0.0 && ( (current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.accept_received_time) > TRANS_ID_MAX_LIFETIME) ||  ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.offer_sent_time > TRANS_ID_MAX_LIFETIME ) ) {
		if ( (current_time.tv_sec + current_time.tv_usec*1e-6 - stl_iterator->st.offer_sent_time) > TRANS_ID_MAX_LIFETIME) {
			 dprintf("LIST TIMEOUT: trans_id %d, offer_sent_time %f, accept_received_time %f\n", stl_iterator->st.trans_id, (double) ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.offer_sent_time  ), (double) ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.accept_received_time));
			 //fprintf(stderr, "LIST TIMEOUT: trans_id %d, offer_sent_time %f, accept_received_time %f\n", stl_iterator->st.trans_id, (double) ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.offer_sent_time  ), (double) ((current_time.tv_sec + current_time.tv_usec*1e-6) - stl_iterator->st.accept_received_time));
			// If it is the first element
			if (stl_iterator->backward == NULL) {
				stl = stl_iterator->forward;
				// Check if I have more than one element in the list
				if (stl_iterator->forward != NULL)
					stl_iterator->forward->backward = NULL;
				stl_iterator->forward = NULL;				
				}
			else { 	// I have to remove the last element of the list
				if (stl_iterator->forward == NULL) {
					stl_iterator->backward->forward = NULL;
					}
				// I have to remove an element in the middle
				else {
					stl_iterator->backward->forward = stl_iterator->forward;
					stl_iterator->forward->backward = stl_iterator->backward;
					}
				}
			something_got_removed = true;
#ifndef MONL
			timeout_reception_measure(stl_iterator->st.id);
#endif

			stl_aux = stl_iterator->forward;
			// Free the memory
			free(stl_iterator);
			}
		if (something_got_removed) {
			stl_iterator = stl_aux;
			something_got_removed = false;
			}
		else
			stl_iterator = stl_iterator->forward;
		}
	return;
}

// register the moment when a transaction is started
// return a  new transaction id
uint16_t transaction_create(struct nodeID *id)
{
	static uint16_t trans_id = 1;

	struct service_times_element *stl2;
	struct timeval current_time;

	check_neighbor_status_list();

	gettimeofday(&current_time, NULL);

	//create new trans_id;
	trans_id++;
	//skip 0
	if (!trans_id) trans_id++;

	// create a new element in the list with its offer_sent_time and set accept_received_time to -1.0
	stl2 = (struct service_times_element*) malloc(sizeof(struct service_times_element));
	stl2->st.trans_id = trans_id;
	stl2->st.offer_sent_time = current_time.tv_sec + current_time.tv_usec*1e-6;
	stl2->st.accept_received_time = -1.0;
	stl2->st.id = id;	//TODO: nodeid_dup
	stl2->backward = NULL;
	if (stl != NULL) {	//List is not empty
		dprintf("LIST: adding trans_id %d to the list, offer_sent_time %f -- LIST IS NOT EMPTY\n", trans_id, (current_time.tv_sec + current_time.tv_usec*1e-6));
		stl2->forward = stl;
		stl->backward = stl2;
		stl = stl2; 
	} else { // This is the first element to insert
		dprintf("LIST: adding trans_id %d to the list, offer_sent_time %f -- LIST IS EMPTY\n", trans_id, current_time.tv_sec + current_time.tv_usec*1e-6);
		stl2->forward = NULL;
		stl = stl2; 
	}
	return trans_id;
}


// Add the moment I received a positive select in a list
// return true if a valid trans_id is found
bool transaction_reg_accept(uint16_t trans_id,const struct nodeID *id)
{
	struct service_times_element *stl_iterator;
	struct timeval current_time;

	gettimeofday(&current_time, NULL);
	stl_iterator = stl;

	// if an accept was received, look for the trans_id and add current_time to accept_received_time field
	dprintf("LIST: changing trans_id %d to the list, accept received %f\n", trans_id, current_time.tv_sec + current_time.tv_usec*1e-6);
	// Iterate the list until you get the right element
	while (stl_iterator != NULL) {
			if (stl_iterator->st.trans_id == trans_id) {
				stl_iterator->st.accept_received_time = current_time.tv_sec + current_time.tv_usec*1e-6;
#ifndef MONL
				offer_accept_rtt_measure(id,stl_iterator->st.accept_received_time - stl_iterator->st.offer_sent_time);
				reception_measure(id);
#endif
				return true;
				}
			stl_iterator = stl_iterator->forward;
	}
	return false;
}

// Used to get the time elapsed from the moment I get a positive select to the moment i get the ACK
// related to the same chunk
// it return -1.0 in case no trans_id is found
double transaction_remove(uint16_t trans_id) {
	struct service_times_element *stl_iterator;
	double to_return;

    dprintf("LIST: deleting trans_id %d\n", trans_id);

	// Start from the beginning of the list
	stl_iterator = stl;
	// Iterate the list until you get the right element
	while (stl_iterator != NULL) {
		if (stl_iterator->st.trans_id == trans_id)
			break;
		stl_iterator = stl_iterator->forward;
		}
	if (stl_iterator == NULL){
		// not found
        dprintf("LIST: deleting trans_id %d -- STL is already NULL \n", trans_id);
		return -2.0;
	}
	
#ifndef MONL
	// This function is called when an ACK is received, so:
	reception_measure(stl_iterator->st.id);
#endif

	to_return = stl_iterator->st.accept_received_time;

	// I have to remove the element from the list
	// If it is the first element
	if (stl_iterator->backward == NULL) {
		stl = stl_iterator->forward;
		// Check if I have more than one element in the list
		if (stl_iterator->forward != NULL)
			stl_iterator->forward->backward = NULL;
		stl_iterator->forward = NULL;
		}
	// I have to remove the last element of the list
	else {
		if (stl_iterator->forward == NULL) {
			stl_iterator->backward->forward = NULL;
			}
		// I have to remove an element in the middle
		else {
			stl_iterator->backward->forward = stl_iterator->forward;
			stl_iterator->forward->backward = stl_iterator->backward;
			}
		}
	free(stl_iterator);
	// Remove RTT measure from queue delay
// 	if (hrc_enabled() && (to_return.accept_received_time > 0.0 && get_measure(to_return.id, 1, MIN) > 0.0 && get_measure(to_return.id, 1, MIN) != NAN))
// 		return (to_return.accept_received_time - get_measure(to_return.id, 1, MIN));

	return to_return;
}
