/******************************************************************************
* myServer.c
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
#include "linkedlist.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void recvFromClient(int clientSocket, linkedList * list);
int checkArgs(int argc, char *argv[]);
void serverControl(int mainServerSocket, linkedList *list);

// function changed
int main(int argc, char *argv[])
{
	int mainServerSocket = 0;   //socket descriptor for the server socket
	int portNumber = 0;
	
	setupPollSet();
	linkedList *list = linkedList_init();
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);
	addToPollSet(mainServerSocket);

	serverControl(mainServerSocket, list);
	
	close(mainServerSocket);

	
	return 0;
}

void serverControl(int mainServerSocket, linkedList *list){

	while(1){
		int clientSocket = 0;
		if ((clientSocket = pollCall(-1)) < 0){ // wait indefinitely and check for error
			printf("Error in pollCall\n");
			exit(-1);
		}
		else if(clientSocket == mainServerSocket){ // new client socket
			addNewSocket(clientSocket);
		}
		else{ // client socket
			processClient(clientSocket, list);
		}
	}
}



int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}