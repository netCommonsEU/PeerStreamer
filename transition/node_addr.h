#ifndef NODE_ADDR_H
#define NODE_ADDR_H

/** @file nade_addr.h
 *
 * @brief transitional file: conversion to new node_addr interface
 *
 * This file serve to convert from the old net_helper interface to the new one in GRAPES 0.3 and above
 * the use of this file is temporary, 
 *
*/
#define NODE_STR_LENGTH 120
const char * node_addr_tr(const struct nodeID *s);

#endif //NODE_ADDR_H
