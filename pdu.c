#include "pdu.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "safeUtil.h"
#include <stdio.h>
#include <stdlib.h>

int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData){  // DO NOT ADD THE FLAG HEADER IN SEND PDU
    
    uint16_t totalLength = lengthOfData + 2;
    uint16_t networkTotalLength = htons(totalLength);

    uint8_t PDU_buffer[totalLength]; // Buffer that will hold the contents that will be sent to the server

    memcpy(PDU_buffer, &networkTotalLength, 2); // Total Length of PDU is added to the buffer
    memcpy(PDU_buffer + 2, dataBuffer, lengthOfData); // The data that will be sent is added to the buffer

    printf("Total length of PDU: %d\n", totalLength);

    int bytesSent = safeSend(clientSocket, PDU_buffer, totalLength, 0);
    printf("bytesSent: %d\n", bytesSent);
    return bytesSent;
}

int recvPDU(int socketNumber, uint8_t *dataBuffer, int bufferSize){   // DO NOT PROCESS THE FLAG HEADER IN RECVPDU
    uint8_t lengthBuffer[2];

    int bytesReceived = safeRecv(socketNumber, lengthBuffer, 2, MSG_WAITALL);
    if(bytesReceived == 0){
        return 0;
    }
    
    uint16_t totalLength = ntohs(*(uint16_t*)lengthBuffer);
    printf("length of received PDU: %d\n", totalLength);
    
    if(totalLength > bufferSize + 2){
        exit(-1);
    }

    bytesReceived = safeRecv(socketNumber, dataBuffer, totalLength - 2, MSG_WAITALL);  // HAS TO BE LENGTH - 2, DONT CHANGE
    printf("data received: %s\n", dataBuffer);
    return bytesReceived;
}