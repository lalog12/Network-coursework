#ifndef __PDULIB_H__
#define __PDULIB_H__

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "safeUtil.h"
#include <stdio.h>
#include <stdint.h>

#define MAXBUF 1024

int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize);


#endif