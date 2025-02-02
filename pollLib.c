//
// Written Hugh Smith, Updated: April 2022
// Use at your own risk.  Feel free to copy, just leave my name in it.
//

// Note this is not a robust implementation 
// 1. It is about as un-thread safe as you can write code.  If you 
//    are using pthreads do NOT use this code.
// 2. pollCall() always returns the lowest available file descriptor 
//    which could cause higher file descriptors to never be processed
//
// This is for student projects so I don't intend on improving this. 

#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include "networks.h"

#include "safeUtil.h"
#include "pollLib.h"
#include "pdu.h"
#include "linkedlist.h"

// Poll global variables 
static struct pollfd * pollFileDescriptors;
static int maxFileDescriptor = 0;
static int currentPollSetSize = 0;

static void growPollSet(int newSetSize);

// Poll functions (setup, add, remove, call)
void setupPollSet()
{
	currentPollSetSize = POLL_SET_SIZE;
	pollFileDescriptors = (struct pollfd *) sCalloc(POLL_SET_SIZE, sizeof(struct pollfd));
}

// *
void addNewSocket(int socketNumber)
{

	int new_socket = tcpAccept(socketNumber, 0);

	if (new_socket > 0)
	{
		addToPollSet(new_socket);
	}
}
// *
void processClient(int socketNumber, linkedList *list){
    uint8_t dataBuffer[MAXBUF];
    int messageLen = 0;

    //now get the data from the client_socket
    if ((messageLen = recvPDU(socketNumber, dataBuffer, MAXBUF)) < 0)
    {
        perror("recv call");
        exit(-1);
    }

	// changes after this line
    if(messageLen == 0)
    {
        printf("Connection closed by other side\n");
        removeFromPollSet(socketNumber);  // Remove from poll set
        close(socketNumber);              // Close the socket
        return;
    }
	else if (messageLen > 0)
    {	
		flag_control(list, dataBuffer, socketNumber);
    }
}

void flag_control(linkedList *list, uint8_t *buffer, int socket){
	uint8_t bufferOffset = 0;
	uint8_t flag = buffer[bufferOffset];
	switch (flag){
		/*packet is flag (1 byte), handle length (1 byte), and 
		  handle with no nulls/padding (handleLen)*/ 
		case 1:{
			uint8_t handleLen = buffer[1];
			char handleBuffer[handleLen + 1];   // + 1 for NULL

			memcpy(handleBuffer, &buffer[2], handleLen);   // copying the handle over to dataBuffer
			buffer[handleLen] = '\0';	
			uint8_t flag = addHandle(list, handleBuffer, socket);

			sendPDU(socket, &flag, 1);
			
			break;
		}

		case 4:{
			printf("Case 4 in flag control in PollLib.c");
		}

		case 5:{														// Flag 5
			uint8_t senderLen = buffer[1];
			char senderName[senderLen + 1]; //handle name and null char
			memcpy(senderName, &buffer[2], senderLen);
			senderName[senderLen] = '\0';

			uint8_t RecipientLen = buffer[3 + senderLen];
			char recipientName[RecipientLen + 1];
			memcpy(recipientName, &buffer[4 + senderLen], RecipientLen);
			recipientName[RecipientLen] = '\0';
			
			int messageStart = 4 + senderLen + RecipientLen;
			
    		char *messageText = (char *)&buffer[messageStart];
			uint8_t messageLen = strlen(messageText);

			int packet_length = messageStart + messageLen + 1;

			Node *DS_handle;
			if ( (DS_handle = lookUphandle(list, recipientName)) == NULL){  // Flag 7
				uint8_t new_flag = 7;
				uint8_t serverBuffer[2 + RecipientLen];
				serverBuffer[0] = new_flag;
				serverBuffer[1] = RecipientLen;
				memcpy(serverBuffer + 2, recipientName, RecipientLen);
				sendPDU(socket, serverBuffer, (2 + RecipientLen));
				break;
			}
			
			else{ // Sending packet to recipient
				sendPDU(DS_handle -> socketNumber, buffer, packet_length);
			}
			
			break;
		}
	case 6: {
			bufferOffset++;
			
			// Get sender information
			uint8_t senderLen = buffer[bufferOffset++];
			char senderName[senderLen + 1];
			memcpy(senderName, &buffer[bufferOffset], senderLen);
			senderName[senderLen] = '\0';
			bufferOffset += senderLen;
			
			// Get number of recipients and setup arrays
			uint8_t numRecipients = buffer[bufferOffset++];
			uint8_t recipientLengths[numRecipients];
			uint8_t recipientOffsets[numRecipients];
			printf("Debug - Initial bufferOffset: %d\n", bufferOffset);
			// Collect recipient handle information
			uint8_t messageOffset = bufferOffset;
			for(int i = 0; i < numRecipients; i++) {
				recipientLengths[i] = buffer[messageOffset];
				recipientOffsets[i] = messageOffset + 1;
				messageOffset += 1 + recipientLengths[i];
				printf("Debug - Recipient %d: length=%d, offset=%d\n", i, recipientLengths[i], recipientOffsets[i]);
			}
			
			// Debug print before message processing
			printf("Debug - Message starts at offset: %d\n", messageOffset);
			printf("Debug - Buffer contents from message start: '");
			for(int i = 0; i < MAXBUF - messageOffset && buffer[messageOffset + i] != '\0'; i++) {
				printf("%c", buffer[messageOffset + i]);
			}
			printf("'\n");
			char *message = (char *)&buffer[messageOffset];
			uint8_t messageLen = strlen(message);
			printf("Debug - Message length: %d\n", messageLen);
			printf("Debug - Final message: '%s'\n", message);
			
			// Process each recipient
			for(int i = 0; i < numRecipients; i++) {
				// Extract recipient name
				char recipientName[recipientLengths[i] + 1];
				memcpy(recipientName, &buffer[recipientOffsets[i]], recipientLengths[i]);
				recipientName[recipientLengths[i]] = '\0';
				
				Node *recipientNode = lookUphandle(list, recipientName);
				
				if(!recipientNode) {
					// Handle not found - send error response
					uint8_t errorResponse[2 + recipientLengths[i]];
					errorResponse[0] = 7;
					errorResponse[1] = recipientLengths[i];
					memcpy(errorResponse + 2, recipientName, recipientLengths[i]);
					sendPDU(socket, errorResponse, 2 + recipientLengths[i]);
					continue;
				}
				
				// Construct and send message packet
				uint8_t packet[MAXBUF];
				uint8_t pktOffset = 0;
				
				// Header and sender info
				packet[pktOffset] = 5;
				pktOffset++;
				packet[pktOffset] = senderLen;
				pktOffset++;
				memcpy(packet + pktOffset, senderName, senderLen);
				pktOffset += senderLen;
				
				// Recipient info
				packet[pktOffset] = 1;
				pktOffset++;
				packet[pktOffset] = recipientLengths[i];
				pktOffset++;

				memcpy(packet + pktOffset, recipientName, recipientLengths[i]);
				pktOffset += recipientLengths[i];
				
				// Message
				memcpy(packet + pktOffset, message, messageLen + 1);
				pktOffset += messageLen + 1;
				
				sendPDU(recipientNode->socketNumber, packet, pktOffset);
			}
			break;
		}
	}
}

void addToPollSet(int socketNumber)
{
	
	if (socketNumber >= currentPollSetSize)
	{
		// needs to increase off of the biggest socket number since
		// the file desc. may grow with files open or sockets
		// so socketNumber could be much bigger than currentPollSetSize
		growPollSet(socketNumber + POLL_SET_SIZE);		
	}
	
	if (socketNumber + 1 >= maxFileDescriptor)
	{
		maxFileDescriptor = socketNumber + 1;
	}

	pollFileDescriptors[socketNumber].fd = socketNumber;
	pollFileDescriptors[socketNumber].events = POLLIN;
}

void removeFromPollSet(int socketNumber)
{
	pollFileDescriptors[socketNumber].fd = 0;
	pollFileDescriptors[socketNumber].events = 0;
}

int pollCall(int timeInMilliSeconds)
{
	// returns the socket number if one is ready for read
	// returns -1 if timeout occurred
	// if timeInMilliSeconds == -1 blocks forever (until a socket ready)
	// (this -1 is a feature of poll)
	// If timeInMilliSeconds == 0 it will return immediately after looking at the poll set
	
	int i = 0;
	int returnValue = -1;
	int pollValue = 0;
	
	if ((pollValue = poll(pollFileDescriptors, maxFileDescriptor, timeInMilliSeconds)) < 0)
	{
		perror("pollCall");
		exit(-1);
	}	
			
	// check to see if timeout occurred (poll returned 0)
	if (pollValue > 0)
	{
		// see which socket is ready
		for (i = 0; i < maxFileDescriptor; i++)
		{
			//if(pollFileDescriptors[i].revents & (POLLIN|POLLHUP|POLLNVAL)) 
			//Could just check for specific revents, but want to catch all of them
			//Otherwise, this could mask an error (eat the error condition)
			if(pollFileDescriptors[i].revents > 0) 
			{
				//printf("for socket %d poll revents: %d\n", i, pollFileDescriptors[i].revents);
				returnValue = i;
				break;
			} 
		}

	}
	
	// Ready socket # or -1 if timeout/none
	return returnValue;
}

static void growPollSet(int newSetSize)
{
	int i = 0;
	
	// just check to see if someone screwed up
	if (newSetSize <= currentPollSetSize)
	{
		printf("Error - current poll set size: %d newSetSize is not greater: %d\n",
			currentPollSetSize, newSetSize);
		exit(-1);
	}
	
	//printf("Increasing poll set from: %d to %d\n", currentPollSetSize, newSetSize);
	pollFileDescriptors = srealloc(pollFileDescriptors, newSetSize * sizeof(struct pollfd));	
	
	// zero out the new poll set elements
	for (i = currentPollSetSize; i < newSetSize; i++)
	{
		pollFileDescriptors[i].fd = 0;
		pollFileDescriptors[i].events = 0;
	}
	
	currentPollSetSize = newSetSize;
}