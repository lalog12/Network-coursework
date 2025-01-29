#include "pdu.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "safeUtil.h"
#include <stdio.h>
#include <stdlib.h>

int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData){
    
    uint16_t length = htons(lengthOfData);
    uint8_t PDU_buffer[2 + lengthOfData];
    memcpy(PDU_buffer, &length, 2);
    memcpy(PDU_buffer + 2, dataBuffer, lengthOfData);
    printf("length of PDU: %d\n", 2 + lengthOfData);
    printf("length network order: %d\n", length);

    int bytesSent = safeSend(clientSocket, PDU_buffer, 2 + lengthOfData, 0);
    printf("bytesSent: %d\n", bytesSent);
    return bytesSent;
}

int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize){
    uint8_t lengthBuffer[2];

    int bytesReceived = safeRecv(socketNumber, lengthBuffer, 2, MSG_WAITALL);
    if(bytesReceived == 0){
        return 0;
    }
    int length = lengthBuffer[0] << 8 | lengthBuffer[1];
    printf("length of PDU: %d\n", length);
    if(length > bufferSize){
        exit(-1);
    }
    bytesReceived = safeRecv(socketNumber, dataBuffer, length, MSG_WAITALL);
    return bytesReceived;
}