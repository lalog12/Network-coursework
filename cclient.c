/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
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
#include "pdu.h"


#define MAXBUF 1024
#define DEBUG_FLAG 1

void recvFromServer(int mainSocket);
void clientControl(int mainSocket, char * handle);
void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void processStdin(int socketNum);
void processMSGFromServer(int mainSocket);
void Initializehandle(int mainSocket, char *handle);



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

		fflush(stdout);  // Enter data: displays immediately
		int clientSocket = 0;

		if ((clientSocket = pollCall(-1)) < 0){ // wait indefinitely and check for error
			printf("Error in pollCall\n");
			exit(-1);
		}
		else if(clientSocket == mainSocket){ // message from server
			processMSGFromServer(mainSocket);
		}
		else if (clientSocket == STDIN_FILENO){
			processStdin(mainSocket);
		}
		else{
			printf("No cases met in clientControl function in cclient.c");
		}
	}
}

void processMSGFromServer(int mainSocket){
	recvFromServer(mainSocket);
}

void processStdin(int socketNum){
	sendToServer(socketNum);
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
		printf("Message received from server on socket %d, length: %d Data: %s\n",mainSocket , messageLen, dataBuffer);
	}
	else
	{
		printf("\nServer terminated\n");
		close(mainSocket);
		removeFromPollSet(mainSocket);
		exit(0);
	}
}



void sendToServer(int socketNum)
{
	uint8_t sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	// sendLen is length of sendBuf
	sendLen = readFromStdin(sendBuf);  
	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
	sent =  sendPDU(socketNum, sendBuf, sendLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("Amount of data sent is: %d\n", sent);
}

// Reads everything from stdin until a \n or buffer limit
// Returns the inputs
int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
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
