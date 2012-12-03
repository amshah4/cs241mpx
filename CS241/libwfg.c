/** @file libwfg.c */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "libwfg.h"
#include "queue.h"


/**
 * Initializes the wait-for graph data structure.
 *
 * This function must always be called first, before any other libwfg
 * functions.
 *
 * @param wfg
 *   The wait-for graph data structure.
 */
void wfg_init(wfg_t *wfg)
{
	wfg->rids=malloc(sizeof(queue_t));
	queue_init(wfg->rids);
	return;
}


/**
 * Adds a "wait" edge to the wait-for graph.
 *
 * This function adds a directed edge from a thread to a resource in the
 * wait-for graph.  If the thread is already waiting on any resource
 * (including the resource) requested, this function should fail.
 *
 * @param wfg
 *   The wait-for graph data structure.
 * @param t_id
 *   A unique identifier to a thread.  A caller to this function may
 *   want to use the thread ID used by the system, by using the
 *   pthread_self() function.
 * @param r_id
 *   A unique identifier to a resource.
 *
 * @return
 *   If successful, this function should return 0.  Otherwise, this
 *   function returns a non-zero value.
 */
int wfg_add_wait_edge(wfg_t *wfg, pthread_t t_id, unsigned int r_id)
{
	int rv=0;
	unsigned int i, j;
	rid_t *tempR=NULL;
	edge_t *tempE=NULL;


	i=0;
	while(i<queue_size(wfg->rids) && tempR==NULL)	//finds correct rid
	{
		tempR=((rid_t*)queue_at(wfg->rids, i));
		if(tempR->rid!=r_id)
		{
			tempR=NULL;
		}


		i++;
	}
	if(tempR!=NULL && tempR->heldBy!=NULL && tempR->heldBy->tid==t_id)	//if rid already exists and is held by this edge
		rv=1;									//we have failed (can't wait on a resource you already have)

	for(i=0; i<queue_size(wfg->rids); i++)
	{
		rid_t *tr=((rid_t*)queue_at(wfg->rids, i));
		for(j=0; j<queue_size(tr->edges); j++)
		{
			if(((edge_t*)queue_at(tr->edges, j))->waitingOnRid!=NULL)//if edge already exists and is waiting on a resource
			{
				rv=1;
			}
		}
	}


	if(rv==0)
	{
		tempE=malloc(sizeof(edge_t));
		tempE->held=0;
		tempE->tid=t_id;

		if(tempR==NULL)
		{
			tempR=malloc(sizeof(rid_t));
			tempR->rid=r_id;
			tempR->heldBy=NULL;
			tempR->edges=malloc(sizeof(queue_t));
			queue_init(tempR->edges);


			queue_enqueue(wfg->rids, ((void*)tempR));
		}


		tempE->waitingOnRid=tempR;
		queue_enqueue(tempR->edges, ((void*)tempE));
	}


	return rv;
}


/**
 * Replaces a "wait" edge with a "hold" edge on a wait-for graph.
 *
 * This function replaces an edge directed from a thread to a resource (a
 * "wait" edge) with an edge directed from the resource to the thread (a
 * "hold" edge).  If the thread does not contain a directed edge to the
 * resource when this function is called, this function should fail.
 * This function should also fail if the resource is already "held"
 * by another thread.
 *
 * @param wfg
 *   The wait-for graph data structure.
 * @param t_id
 *   A unique identifier to a thread.  A caller to this function may
 *   want to use the thread ID used by the system, by using the
 *   pthread_self() function.
 * @param r_id
 *   A unique identifier to a resource.
 *
 * @return
 *   If successful, this function should return 0.  Otherwise, this
 *   function returns a non-zero value.
 */
int wfg_add_hold_edge(wfg_t *wfg, pthread_t t_id, unsigned int r_id)
{
	unsigned int i;
	int rv=1;
	rid_t *tempR=NULL;


	i=0;
	while(i<queue_size(wfg->rids) && tempR==NULL)	//finds correct rid
	{
		tempR=((rid_t*)queue_at(wfg->rids, i));
		if(tempR->rid!=r_id)
		{
			tempR=NULL;
		}


		i++;
	}

	if(tempR!=NULL && tempR->heldBy==NULL)	//if rid found and isn't already held
	{
		i=0;
		while(i<queue_size(tempR->edges) && rv!=0)	//finds correct edge
		{
			tempR->heldBy=((edge_t*)queue_at(tempR->edges, i));
			if(tempR->heldBy->tid==t_id && tempR->heldBy->waitingOnRid->rid==r_id)
			{
				tempR->heldBy->held=1;
				tempR->heldBy->waitingOnRid=NULL;


				rv=0;
			}
			else
			{
				tempR->heldBy=NULL;
			}


			i++;
		}
	}


	return rv;
}


/**
 * Removes an edge on the wait-for graph.
 *
 * If any edge exists between the thread and resource, this function
 * removes that edge (either a "hold" edge or a "wait" edge).
 *
 * @param wfg
 *   The wait-for graph data structure.
 * @param t_id
 *   A unique identifier to a thread.  A caller to this function may
 *   want to use the thread ID used by the system, by using the
 *   pthread_self() function.
 * @param r_id
 *   A unique identifier to a resource.
 *
 * @return
 * - 0, if an edge was removed
 * - non-zero, if no edge was removed
 */
int wfg_remove_edge(wfg_t *wfg, pthread_t t_id, unsigned int r_id)
{
	int rv=1;
	unsigned int i=0;
	rid_t *tempR=NULL;
	edge_t *tempE=NULL;


	while(i<queue_size(wfg->rids) && tempR==NULL)	//finds correct rid
	{
		tempR=((rid_t*)queue_at(wfg->rids, i));
		if(tempR->rid!=r_id)
		{
			tempR=NULL;
		}


		i++;
	}

	if(tempR!=NULL)	//if found
	{
		i=0;
		while(i<queue_size(tempR->edges) && rv!=0)
		{
			tempE=((edge_t*)queue_at(tempR->edges, i));
			if(tempE->tid==t_id)
			{
				if(tempR->heldBy->tid==((edge_t*)queue_remove_at(tempR->edges, i))->tid)
				{
					tempR->heldBy=NULL;
				}


				free(tempE);
				rv=0;
			}


			i++;
		}
	}


	return rv;
}


/**
 * Returns the numebr of threads and the identifiers of each thread that is
 * contained in a cycle on the wait-for graph.
 *
 * If the wait-for graph contains a cycle, this function allocates an array
 * of unsigned ints equal to the size of the cycle, populates the array with
 * the thread identifiers of the threads in the cycle, modifies the value
 * of <tt>cycle</tt> to point to this newly created array, and returns
 * the length of the array.
 *
 * For example, if a cycle exists such that:
 *    <tt>t(id=1) => r(id=100) => t(id=2) => r(id=101) => t(id=1) (cycle)</tt>
 * ...the array will be of length two and contain the elements: [1, 2].
 *
 * This function only returns a single cycle and may return the same cycle
 * each time the function is called until the cycle has been removed.  This
 * function can return ANY cycle, but it must contain only threads in the
 * cycle itself.
 *
 * It is up to the user of this library to free the memory allocated by this
 * function.
 *
 * If no cycle exists, this function must not allocate any memory and will
 * return 0.
 *
 *
 * @param wfg
 *   The wait-for graph data structure.
 * @param cycle
 *   A pointer to an (unsigned int *), used by the function to return the
 *   cycle in the wait-for graph if a cycle exists.
 *
 * @return
 *   The number of threads in the identified cycle in the wait-for graph,
 *   or 0 if no cycle exists.
 */
int wfg_get_cycle(wfg_t *wfg, pthread_t** cycle)
{
	int count=0;





	return count;
}


/**
 * Prints all wait-for relations between threads in the wait-for graph.
 *
 * In a wait-for graph, a thread is "waiting for" another thread if a
 * thread has a "wait" edge on a resource that is being "held" by another
 * process.  This function prints all of the edges to stdout in a comma-
 * seperated list in the following format:
 *    <tt>[thread]=>[thread], [thread]=>[thread]</tt>
 *
 * If t(id=1) and t(id=2) are both waiting for r(id=100), but (r=100)
 * is held by t(id=3), the printout should be:
 *    <tt>1=>3, 2=>3</tt>
 *
 * When printing out the wait-for relations:
 * - this function may list the relations in any order,
 * - all relations must be printed on one line,
 * - all relations must be seperated by comma and a space (see above),
 * - a newline should be placed at the end of the list, and
 * - you may have an extra trailing ", " if it's easier for your program
 *   (not required, but should save you a bit of coding).
 *
 * @param wfg
 *   The wait-for graph data structure.
 */
void wfg_print_graph(wfg_t *wfg)
{

}


/**
 * Destroys the wait-for graph data structure.
 *
 * This function must always be called last, after all other libwfg functions.
 *
 * @param wfg
 *   The wait-for graph data structure.
 */
void wfg_destroy(wfg_t *wfg)
{
	while(queue_size(wfg->rids))
	{
		rid_t *tempR=queue_dequeue(wfg->rids);	//for each resource
		while(queue_size(tempR->edges))				//delete all associated edges
		{
			free(queue_dequeue(tempR->edges));
		}
		queue_destroy(tempR->edges);				//destroy edge queue
		free(tempR->edges);

		free(tempR);							//delete the resource that now has no associated edges
	}
	queue_destroy(wfg->rids);
	free(wfg->rids);


	return;
}
