/** @file server.c */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <queue.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "queue.h"
#include "libdictionary.h"


//GLOBALS
const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";

struct addrinfo *hints, *result;
queue_t *tids, *clients;


//FUNCTION PROTOTYPES
void* handleConnection(void* nothing);


/**
 * Processes the request line of the HTTP header.
 *
 * @param request The request line of the HTTP header.  This should be
 *                the first line of an HTTP request header and must
 *                NOT include the HTTP line terminator ("\r\n").
 *
 * @return The filename of the requested document or NULL if the
 *         request is not supported by the server.  If a filename
 *         is returned, the string must be free'd by a call to free().
 */
char* process_http_header_request(const char *request)
{
	// Ensure our request type is correct...
	if (strncmp(request, "GET ", 4) != 0)
		return NULL;

	// Ensure the function was called properly...
	assert( strstr(request, "\r") == NULL );
	assert( strstr(request, "\n") == NULL );

	// Find the length, minus "GET "(4) and " HTTP/1.1"(9)...
	int len = strlen(request) - 4 - 9;

	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + 4, len);
	filename[len] = '\0';

	// Prevent a directory attack...
	//  (You don't want someone to go to http://server:1234/../server.c to view your source code.)
	if (strstr(filename, ".."))
	{
		free(filename);
		return NULL;
	}

	return filename;
}


int main(int argc, char **argv)
{
	int sockfd;
	tids=malloc(sizeof(queue_t));
	clients=malloc(sizeof(queue_t));
	queue_init(tids);
	queue_init(clients);


	//PART 1
	sockfd=socket(AF_INET/*ipv4*/, SOCK_STREAM/*TCP*/, 0);
	if(sockfd==-1)
	{
		perror("socket() failed\n");
		exit(1);
	}

	//creates struct for bind;
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_PASSIVE;
	if(getaddrinfo(NULL, argv[1]/*port*/, hints, &result)!=0)
	{
		perror("getadderinfo() failed\n");
		exit(1);
	}

	if(bind(sockfd, result->ai_addr, result->ai_addrlen) != 0)
	{
		perror("bind\n");
		exit(1);
	}

	if(listen(sockfd, 10)!=0)
	{
		perror("listen() failed\n");
		exit(1);
	}


	//PART 2
	while(1)
	{
		pthread_t *temp=malloc(sizeof(pthread_t));
		int *tempfd=malloc(sizeof(int));

		*tempfd=accept(sockfd, NULL, NULL);
		pthread_create(temp, NULL, handleConnection, ((void*)tempfd));

		queue_enqueue(tids, ((void*)temp));
		queue_enqueue(clients, ((void*)tempfd));
	}


	return 0;
}


//PART 3
/* function for pthread_create() to call every time connection is
 * accepted. Consists of part 3 of MP8
 *
 * @param - pointer to an integer returned by accept representing a
 * 			new socket fd to send()/recv() from
 * @return - NULL
 */
void* handleConnection(void *clifd)
{
	int clientfd=*((int*)clifd);
	int keepReceiving, connectionIsAlive=1;	//bools
	unsigned int len;


	while(connectionIsAlive)
	{
		len=0;
		keepReceiving=1;
		char buffer[4096];


		//PART 3a
		while(keepReceiving)
		{
			len+=recv(clientfd, &(buffer[len]), 4096-len, 0);

			if(len>=4 &&	buf[len-4]=='\r' &&
							buf[len-3]=='\n' &&
							buf[len-2]=='\r' &&
							buf[len-1]=='\n')//if finished receiving, end loop
			{
				keepReceiving=0;
			}
			if(len==0)//if recv() returned 0, connection was closed
			{
				keepReceiving=0;
				connectionIsAlive=0;
			}
		}
		buffer[len]='\0';


		if(connectionIsAlive)
		{
			int i=0, j;				//indexes
			int notEOLine=1;
			queue_t *parsedHeader;
			queue_init(&parsedHeader);


			//PART 3b
			while(i<len)
			{
				char lineBuffer[4096];
				j=0;


				while(notEOLine)
				{
					lineBuffer[j]=buffer[i];
					if(lineBuffer[j]=='\n')
						notEOLine=0;
					j++;
					i++;
				}
				lineBuffer[j]='\0';

				char *temp=malloc(1024*sizeof(char));
				if(queue_size(&parsedHeader)==0)
				{
					strcpy(temp, process_http_header_request(lineBuffer));
				}
				else
				{
					strcpy(temp, lineBuffer);
				}
				queue_enqueue(&parsedHeader, temp);
			}


			//PART 3c





			//CLEANUP
			while(queue_size(&parsedHeader))
			{
				free(queue_dequeue(&parsedHeader));
			}
			queue_destroy(&parsedHeader);
		}
	}


	return NULL;
}