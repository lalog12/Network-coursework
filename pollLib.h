// 
// Writen by Hugh Smith, April 2022
//
// Provides an interface to the poll() library.  Allows for
// adding a file descriptor to the set, removing one and calling poll.
// Feel free to copy, just leave my name in it, use at your own risk.
//


#ifndef __POLLLIB_H__
#define __POLLLIB_H__

#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include "networks.h"

#include "safeUtil.h"
#include "pollLib.h"
#include "pdu.h"
#include "linkedlist.h"

#define POLL_SET_SIZE 10
#define POLL_WAIT_FOREVER -1

void setupPollSet();
void addToPollSet(int socketNumber);
void removeFromPollSet(int socketNumber);
int pollCall(int timeInMilliSeconds);

void addNewSocket(int socketNumber);
void processClient(int socketNumber, linkedList *list);
void flag_control(linkedList *list, uint8_t *buffer, int socket);

#endif