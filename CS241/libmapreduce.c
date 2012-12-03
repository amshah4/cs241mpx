/** @file libmapreduce.c */
/*
 * CS 241
 * The University of Illinois
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>

#include "libmapreduce.h"
#include "libdictionary.h"


typedef struct _thread_data_t
{
	mapreduce_t *mr;
	int **fds;
	int numValues;

}thread_data_t;


static const int BUFFER_SIZE = 2048;  /**< Size of the buffer used by read_from_fd(). */


//prototype
void* WorkerThread(void *fdData);

/**
 * Adds the key-value pair to the mapreduce data structure.  This may
 * require a reduce() operation.
 *
 * @param key
 *    The key of the key-value pair.  The key has been malloc()'d by
 *    read_from_fd() and must be free()'d by you at some point.
 * @param value
 *    The value of the key-value pair.  The value has been malloc()'d
 *    by read_from_fd() and must be free()'d by you at some point.
 * @param mr
 *    The pass-through mapreduce data structure (from read_from_fd()).
 */
static void process_key_value(const char *key, const char *value, mapreduce_t *mr)
{
	if(dictionary_add(mr->data, key, value)==KEY_EXISTS)
	{
		char *temp=mr->reduce(dictionary_get(key), value);
		dictionary_remove_free(mr->data, key);
		dictionary_add(mr->data, key, temp);
		free(value);
	}


	return;
}


/**
 * Helper function.  Reads up to BUFFER_SIZE from a file descriptor into a
 * buffer and calls process_key_value() when for each and every key-value
 * pair that is read from the file descriptor.
 *
 * Each key-value must be in a "Key: Value" format, identical to MP1, and
 * each pair must be terminated by a newline ('\n').
 *
 * Each unique file descriptor must have a unique buffer and the buffer
 * must be of size (BUFFER_SIZE + 1).  Therefore, if you have two
 * unique file descriptors, you must have two buffers that each have
 * been malloc()'d to size (BUFFER_SIZE + 1).
 *
 * Note that read_from_fd() makes a read() call and will block if the
 * fd does not have data ready to be read.  This function is complete
 * and does not need to be modified as part of this MP.
 *
 * @param fd
 *    File descriptor to read from.
 * @param buffer
 *    A unique buffer associated with the fd.  This buffer may have
 *    a partial key-value pair between calls to read_from_fd() and
 *    must not be modified outside the context of read_from_fd().
 * @param mr
 *    Pass-through mapreduce_t structure (to process_key_value()).
 *
 * @retval 1
 *    Data was available and was read successfully.
 * @retval 0
 *    The file descriptor fd has been closed, no more data to read.
 * @retval -1
 *    The call to read() produced an error.
 */
static int read_from_fd(int fd, char *buffer, mapreduce_t *mr)
{
	/* Find the end of the string. */
	int offset = strlen(buffer);

	/* Read bytes from the underlying stream. */
	int bytes_read = read(fd, buffer + offset, BUFFER_SIZE - offset);
	if (bytes_read == 0)
		return 0;
	else if(bytes_read < 0)
	{
		fprintf(stderr, "error in read.\n");
		return -1;
	}

	buffer[offset + bytes_read] = '\0';

	/* Loop through each "key: value\n" line from the fd. */
	char *line;
	while ((line = strstr(buffer, "\n")) != NULL)
	{
		*line = '\0';

		/* Find the key/value split. */
		char *split = strstr(buffer, ": ");
		if (split == NULL)
			continue;

		/* Allocate and assign memory */
		char *key = malloc((split - buffer + 1) * sizeof(char));
		char *value = malloc((strlen(split) - 2 + 1) * sizeof(char));

		strncpy(key, buffer, split - buffer);
		key[split - buffer] = '\0';

		strcpy(value, split + 2);

		/* Process the key/value. */
		process_key_value(key, value, mr);

		/* Shift the contents of the buffer to remove the space used by the processed line. */
		memmove(buffer, line + 1, BUFFER_SIZE - ((line + 1) - buffer));
		buffer[BUFFER_SIZE - ((line + 1) - buffer)] = '\0';
	}

	return 1;
}


/**
 * Initialize the mapreduce data structure, given a map and a reduce
 * function pointer.
 */
void mapreduce_init(mapreduce_t *mr,
                    void (*mymap)(int, const char *),
                    const char *(*myreduce)(const char *, const char *))
{
	mr->map=mymap;
	mr->reduce=myreduce;
	mr->lock=malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mr->lock);
	mr->data=malloc(sizeof(dictionary_t));
	dictionary_init(mr->data);


	return;
}


/**
 * Starts the map() processes for each value in the values array.
 * (See the MP description for full details.)
 */
void mapreduce_map_all(mapreduce_t *mr, const char **values)
{
	pthread_mutex_lock(mr->lock);
	int i;
	int numVals=0;
	while(values[numVals]!=NULL)
	{
		numVals++;
	}
	int **fds=malloc(numVals*sizeof(int*));
	for(i=0; i<numVals; i++)
	{
		fds[i]=malloc(2*sizeof(int));					//0 for read, 1 for write
		pipe(fds[i]);
	}


	for(i=0; i<numVals; i++)
	{
		if(fork()==0)
		{
			map(fds[i][1], values[i]);
			exit(0);
		}
	}


	pthread_t dummy=malloc(sizeof(pthread_t));
	thread_data_t *input=malloc(sizeof(thread_data_t));
	input->mr=mr;
	input->fds=fds;
	input->numValues=numVals;
	pthread_create(&dummy, NULL, WorkerThread, input);


	return;
}
void* WorkerThread(void *fdData)
{
	thread_data_t *dat=(thread_data_t)fdData;
	char *buf=malloc(sizeof(BUFFER_SIZE+1));
	int eof=0;
	int i;
	fd_set readSelect;

	while(!eof)
	{
		FD_ZERO(&readSelect);
		for(i=0; i<dat->numValues; i++)
		{
			FD_SET(dat->fds[i], &readSelect);
		}

		if(select(dat->numValues, &readSelect, NULL, NULL, NULL))
		{
			for(i=0; i<dat->numValues; i++)
			{
				if(FD_ISSET(dat->fds[i], &readSelect))
				{
					read_from_fd(dat->fds[i][0], buf, dat->mr);
					if(strcmp(buf, EOF)==0)
					{
						i=dat->numValues;
						eof=1;
					}
				}
			}
		}
	}


	pthread_mutex_unlock(dat->mr->lock);
	free(dat->fds);
	free(dat);
	return NULL;
}


/**
 * Blocks until all the reduce() operations have been completed.
 * (See the MP description for full details.)
 */
void mapreduce_reduce_all(mapreduce_t *mr)
{
	pthread_mutex_lock(mr->lock);		//should block until thread unlocks
	pthread_mutex_unlock(mr->lock);
	return;
}


/**
 * Gets the current value for a key.
 * (See the MP description for full details.)
 */
const char *mapreduce_get_value(mapreduce_t *mr, const char *result_key)
{
	return dictionary_get(mr->data, result_key);
}


/**
 * Destroys the mapreduce data structure.
 */
void mapreduce_destroy(mapreduce_t *mr)
{
	pthread_mutex_destroy(mr->lock);
	free(mr->lock);
	dictionary_destroy_free(mr->data);
	free(mr->data);
	return;
}

