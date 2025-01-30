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
void clientControl(int mainSocket);
void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void processStdin(int socketNum);
void processMSGFromServer(int mainSocket);

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	
	checkArgs(argc, argv);
	
	/* set up the TCP Client socket  */  
	socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG); // server name and port number
	while(1){
		clientControl(socketNum);
	}
	close(socketNum);
	
	return 0;
}

void clientControl(int mainSocket){
	setupPollSet();
	addToPollSet(mainSocket);
	addToPollSet(STDIN_FILENO);

	while(1){
		printf("Enter data: ");
		fflush(stdout);
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
	if (argc != 3)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
}
