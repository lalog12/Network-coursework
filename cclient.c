/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include "pdu.h"
#include "networks.h"
#include "safeUtil.h"
#include "pollLib.h"


#define MAXBUF 1024
#define DEBUG_FLAG 1

void recvFromServer(int mainSocket);
void clientControl(int mainSocket, char * handle);
void sendToServer(int socketNum, char *handle);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void processStdin(int socketNum, char *handle);
void processMSGFromServer(int mainSocket);
void Initializehandle(int mainSocket, char *handle);
int8_t flag_converter(uint8_t * unaltered_buffer);
void flag_data_constructer(int8_t flag, uint8_t *StdinBuf, uint8_t *sendBuf,  char *clienthandle);



int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	
	checkArgs(argc, argv);    
	
	/* set up the TCP Client socket  */  
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG); // server name and port number

	clientControl(socketNum, argv[1]);
	close(socketNum);
	
	return 0;
}


void Initializehandle(int mainSocket, char *handle){

	if(!handle){
		fprintf(stderr, "Error: No handle parameter\n");
		exit(-1);
	}

	uint8_t handleLen = strlen(handle);

	if (handleLen > 100){
		fprintf(stderr, "Invalid handle, handle longer than 100 characters: %s\n", handle);
		exit(-1);
	}
	// packet is flag (1 byte), handle length (1 byte), and handle with no nulls/padding (handleLen)
	uint16_t bufLen = 1 + 1 + handleLen;
	uint8_t buf[bufLen];
	uint8_t flag = 1;

	memcpy(buf, &flag, 1);
	memcpy(buf + 1, &handleLen, 1);
	memcpy(buf + 2, handle, handleLen);

	sendPDU(mainSocket, buf, bufLen);

	pollCall(-1);

	uint8_t dataBuffer[1];
	
	int messageLen;

	if ((messageLen = recvPDU(mainSocket, dataBuffer, 1)) < 0)
	{
		perror("recv call");
		close(mainSocket);
		removeFromPollSet(mainSocket);
		exit(-1);
	}

	flag = dataBuffer[0];

	if (flag == 2){
		printf("Successfully added handle to Handle Table: %s\n", handle);
		return; 
	}
	else if(flag == 3){
		fprintf(stderr, "Handle already in use: %s\n", handle);
		exit(-1);
	}
	else{
		fprintf(stderr, "Flag does not meet requirements. Flag should be 2 or 3 and it is: %d\n", flag);
		exit(-1);
	}
	
}

void clientControl(int mainSocket, char * handle){
	setupPollSet();
	addToPollSet(mainSocket);
	addToPollSet(STDIN_FILENO);

	Initializehandle(mainSocket, handle);

	while(1){
		printf("$:");

		fflush(stdout);  // $: displays immediately
		int clientSocket = 0;

		if ((clientSocket = pollCall(-1)) < 0){ // wait indefinitely and check for error
			printf("Error in pollCall\n");
			exit(-1);
		}
		else if(clientSocket == mainSocket){ // message from server
			processMSGFromServer(mainSocket);
		}
		else if (clientSocket == STDIN_FILENO){  // message from client terminal
			processStdin(mainSocket, handle);
		}
		else{
			printf("No cases met in clientControl function in cclient.c");
		}
	}
}

void processMSGFromServer(int mainSocket){
	recvFromServer(mainSocket);
}

void processStdin(int socketNum, char *handle){
	sendToServer(socketNum, handle);
}

void recvFromServer(int mainSocket)
{
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;

	//now get the data from the client_socket
	if ((messageLen = recvPDU(mainSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		close(mainSocket);
		removeFromPollSet(mainSocket);
	}

	if (messageLen > 0)
	{
		uint8_t sendLen = dataBuffer[1];

		char senderName[sendLen + 1];
		memcpy(senderName, &dataBuffer[2], sendLen);
		senderName[sendLen] = '\0';


		uint8_t recipientLen = dataBuffer[3 + sendLen];
		
		// Message starts after recipient's handle
		int messageStart = 4 + sendLen + recipientLen;
		char *messageText = (char *)&dataBuffer[messageStart];
		
		// Print in required format
		printf("%s: %s\n", senderName, messageText);
		printf("Message received from server on socket %d, length: %d Data: %s\n", mainSocket, messageLen, dataBuffer);
	}
	else
	{
		printf("\nServer terminated\n");
		close(mainSocket);
		removeFromPollSet(mainSocket);
		exit(0);
	}
}

void flag_data_constructer(int8_t flag, uint8_t *StdinBuf, uint8_t *sendBuf,  char *clienthandle){
	
	switch(flag){
		
		case 4:   //broadcast %b

		break;

		case 5:{   // send message to single client %m

			uint8_t strLen = strlen(clienthandle);  // Store length

			sendBuf[0] = 5;     // flag

			sendBuf[1] = strLen;

			memcpy(sendBuf + 2, clienthandle, strLen);

			sendBuf[2 + strLen] = 1;    
					
			strtok((char *)StdinBuf, " ");        // skipping flag
			char *handle = strtok(NULL, " ");  // getting second word from stdin
					
			sendBuf[3 + strLen] = strlen(handle);

			memcpy(sendBuf + 4 + strLen, handle, strlen(handle));
			
			int sendBufPos = 4 + strLen + strlen(handle);
			int StdinBufPos = 4 + strlen(handle);

			while (StdinBuf[StdinBufPos] != '\0'){
				sendBuf[sendBufPos] = StdinBuf[StdinBufPos];
				sendBufPos++;
				StdinBufPos++;
			} 
			sendBuf[sendBufPos] = '\0';

			break;
		}

		case 6: {    // send message to group of clients
			uint8_t offset = 0;
			sendBuf[offset] = 6;    // flag          xxxxxxxxxxxxxxx
			offset++;
			
			uint8_t clientLen = strlen(clienthandle) ;  // 
			sendBuf[offset] = clientLen;     //client len    xxxxxxxxxxx
			offset++;

			memcpy(sendBuf + offset, clienthandle, clientLen);    // sending client's handle
			offset += clientLen;

			strtok((char *)StdinBuf, " ");  // Skip %C

			char *numStr = strtok(NULL, " ");  // Get number
    		if (numStr == NULL) {
				printf("Invalid command format\n");
				break;
    		}
			char *endptr;
    		long numRecipients = strtol(numStr, &endptr, 10);

			if (numRecipients < 2 || numRecipients > 9){
				printf("Invalid number of recipients (must be between 2 and 9)\n");
				break;
			}
			uint8_t numRecipients8 = numRecipients;
			sendBuf[offset] = numRecipients8;   // # of destination handles
			offset++;

			uint8_t cnt = 0;
			while(cnt < numRecipients8){
				char *dest_handle = strtok(NULL, " ");  // Changed to NULL
				uint8_t len = strlen(dest_handle);
				sendBuf[offset] = len;
				offset++;
				memcpy(sendBuf + offset, dest_handle, len );
				offset = offset + len;
				cnt++;
			}

			// Get the rest of the message after the last handle
			char *message = strtok(NULL, "\0");  // Changed delimiter to \0 to get rest of string
			
			uint8_t messageLen = strlen(message);
			memcpy(sendBuf + offset, message, messageLen + 1);  // +1 to include null terminator
			offset = offset + messageLen + 1;

	

			break;

		case 7: 
			printf("Client with handle 123456 does not exist" );

		case 10:

		break;
	}
}
}

void sendToServer(int socketNum, char *handle)
{	
	uint8_t StdinBuf[MAXBUF];   // terminal buffer
	uint8_t sendBuf[MAXBUF];	// buffer to send to server
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */

	// sendLen is length of sendBuf
	sendLen = readFromStdin(StdinBuf);
	printf("read: %s string len: %d (including null)\n", StdinBuf, sendLen);
	
	int8_t flag = flag_converter(StdinBuf);
	if (flag < 0){ // Invalid command inputted on terminal
		return;
	}
	printf("flag: %d\n", flag);
	flag_data_constructer(flag, StdinBuf, sendBuf, handle);

	uint8_t sendBufLen = strlen((char*)sendBuf);
	printf("Buffer being sent to server: %s\n", sendBuf);
	sent =  sendPDU(socketNum, sendBuf, sendBufLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}
	printf("Amount of data sent is: %d\n", sent);
}

int8_t flag_converter(uint8_t * unaltered_buffer){
	char unfiltered_flag[3];
	memcpy(unfiltered_flag, unaltered_buffer, 2);
	unfiltered_flag[1] = tolower(unfiltered_flag[1]); //might cause bug
	unfiltered_flag[2] = '\0';

	if(strcmp(unfiltered_flag, "%m") == 0){
		return 5;
	}

	else if (strcmp(unfiltered_flag, "%b") == 0){
		return 4;
	}

	else if (strcmp(unfiltered_flag, "%c") == 0){
		return 6;
	}

	else if(strcmp(unfiltered_flag, "%l") == 0){
		return 10;
	}
	
	else{
		printf("Invalid command\n");
		return -1;
	}

}

// Reads everything from stdin until a \n or buffer limit
// Returns the inputs
int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	

	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle-name host-name port-number \n", argv[0]);
		exit(-1);
	}
}
