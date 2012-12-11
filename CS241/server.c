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
pthread_mutex_t *lock;


//FUNCTION PROTOTYPES
void* handleConnection(void* nothing);
void handlecc(int sig);


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


	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	lock=malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(lock, &attr);
	pthread_mutexattr_destroy(&attr);


	struct sigaction cc;	//ctrl+c handlerer
	cc.sa_handler=handlecc;
	cc.sa_flags=0;
	sigemptyset(&cc.sa_mask);
	sigaction(SIGINT, &cc, NULL);


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
				if(queue_size(&parsedHeader)==0)//if request line
				{
					strcpy(temp, process_http_header_request(lineBuffer));
				}
				else
				{
					strcpy(temp, lineBuffer);
				}
				queue_enqueue(&parsedHeader, ((void*)temp));
			}


			//PART 3cdef
			char filename[1024];
			filename[0]='\0';
			strcat(filename, "web");
			FILE *file=NULL;

			char responseln[1024];
			responseln[0]='\0';
			char contentType[1024];
			contentType[0]='\0';
			char contentLength[1024];
			contentLength[0]='\0';
			char connection[1024];
			connection[0]='\0';
			char *content=malloc(4096*sizeof(char));
			content[0]='\0';


			//Set responseln, contentType, contentLength, and content
			if(((char*)queue_at(&parsedHeader, 0))==NULL)	//501 Response
			{
				//501
				sprintf(responseln, "HTTP/1.1 501 %s\r\n", HTTP_501_STRING);
				strcat(contentType, "Content-Type: text/html\r\n");
				sprintf(contentLength, "Content-Length: %d\r\n", strlen(HTTP_501_CONTENT));
				strcat(content, HTTP_501_CONTENT);
			}
			else
			{
				if(strcmp(((char*)queue_at(&parsedHeader, 0)), "/")==0)
				{
					strcat(filename, "/index.html");
				}
				else
				{
					strcat(filename, ((char*)queue_at(&parsedHeader, 0)));
				}
				file=fopen(filename, "r");

				if(file==NULL)
				{
					//404
					sprintf(responseln, "HTTP/1.1 404 %s\r\n", HTTP_404_STRING);
					strcat(contentType, "Content-Type: text/html\r\n");
					sprintf(contentLength, "Content-Length: %d\r\n", strlen(HTTP_404_CONTENT));
					strcat(content, HTTP_404_CONTENT);
				}
				else
				{
					//200
					sprintf(responseln, "HTTP/1.1 200 %s", HTTP_200_STRING);

					int size=strlen(filename);
					char extention[4];
					extention[0]=filename[size-3];
					extention[1]=filename[size-2];
					extention[2]=filename[size-1];
					extention[3]='\0';
					if(strcmp(extention, "tml")==0)
					{
						strcat(contentType, "Content-Type: text/html\r\n");
					}
					else if(strcmp(extention, "css")==0)
					{
						strcat(contentType, "Content-Type: text/css\r\n");
					}
					else if(strcmp(extention, "jpg")==0)
					{
						strcat(contentType, "Content-Type: image/jpeg\r\n");
					}
					else if(strcmp(extention, "png")==0)
					{
						strcat(contentType, "Content-Type: image/png\r\n");
					}
					else
					{
						strcat(contentType, "Content-Type: text/plain\r\n");
					}

					fseek(file, 0, SEEK_END);
					size=ftell(file);
					fseek(file, 0, SEEK_SET);
					sprintf(contentLength, "Content-Length: %d\r\n", size);

					free(content);
					content=malloc((size+1)*sizeof(char));
					content[0]='\0';
					char line[size];
					while(fgets(line, size, file) != NULL)
					{
						strcat(content, line);
					}
				}
			}

			int i=0;
			while(i<queue_size(&parsedHeader))
			{
				if(strcasecmp(((char*)queue_at(&parsedHeader, i)), "Connection: Keep-Alive\r\n")==0)
				{
					strcat(connection, "Connection: Keep-Alive\r\n\r\n");
				}


				i++;
			}
			if(strlen(connection)==0)
			{
				strcat(connection, "Connection: close\r\n\r\n");
				connectionIsAlive=0;
			}


			//At this point, all parts of the packet are finished. Time to combine and send()
			char *packet=malloc((	strlen(responseln) +
									strlen(contentType) +
									strlen(contentLength) +
									strlen(connection) +
									strlen(content) +
									1)*sizeof(char));
			sprintf(packet, "%s%s%s%s%s", responseln, contentType, contentLength, connection, content);
			send(clientfd, packet, strlen(packet)*sizeof(char), 0);


			//CLEANUP
			while(queue_size(&parsedHeader))
			{
				free(queue_dequeue(&parsedHeader));
			}
			queue_destroy(&parsedHeader);
			free(content);
			free(packet);
		}
	}


	close(clientfd);
	int i;

	//CRITICAL ZONE
	pthread_mutex_lock(lock);
	for(i=0; i<queue_size(clients); i++)
	{
		if(*((int*)queue_at(clients, i))==clientfd)
		{
			free(queue_remove_at(clients, i));
		}
	}
	pthread_mutex_unlock(lock);
	//END CRITICAL ZONE

	return NULL;
}


//PART 4
/* callback function to catch and handle ctrl+c
 *
 * @param - int signal
 * @return - all threads terminate, each closing their socket, and any memory left unfreed is now freed
 */
void handlecc(int sig)
{
	free(hints);
	free(result);


	while(queue_size(clients))
	{
		int *temp=queue_dequeue(clients);
		close(*temp);
		free(temp);
	}
	while(queue_size(tids))
	{
		pthread_t *tid=queue_dequeue(tids);
		void **dummy;
		pthread_join(*tid, dummy);
	}
	queue_destroy(tids);
	queue_destroy(clients);
	free(tids);
	free(clients);
	pthread_mutex_destroy(lock);
	free(lock);


	exit(0);
	return;		//should never be reached
}
