/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.

  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
	q->compare=comparer;	//Decision: negative number means first element is lower priority
	q->size=0;
	q->head=malloc(sizeof(priqueueElement_t));

	q->head->data=(void*)("Dummy Node");
	q->head->next=NULL;


	return;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr
   	   	   	   	   	   	   was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
	int done=0;			//bool
	int index=0;
	priqueueElement_t *current=q->head;


	while(!done)		//NOTE: could use forloop and priqueue_at, but inefficient
	{
		if(current->next==NULL)
		{
			current->next=malloc(sizeof(priqueueElement_t));
			current->next->data=ptr;
			current->next->next=NULL;

			done=1;
		}
		else if(q->compare(ptr, current->next->data)<=0)
		{
			priqueueElement_t *temp=current->next;
			current->next=malloc(sizeof(priqueueElement_t));
			current->next->data=ptr;
			current->next->next=temp;

			done=1;
		}
		else
		{
			current=current->next;
			index++;
		}
	}


	q->size++;
	return index;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	return q->head->next;
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
	return priqueue_remove_at(q, 0);
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	int i;
	void *rv=NULL;


	if(0<=index && index<q->size)
	{
		priqueueElement_t *current=q->head;
		for(i=0; i<index; i++)
		{
			current=current->next;
		}
		rv=current->next->data;
	}


	return rv;
}


/**
  Removes all instances of ptr from the queue.

  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
	int i;
	int removed=0;
	priqueueElement_t *current=q->head;


	for(i=0; i<q->size; i++)
	{
		if(current->next->data==ptr)
		{
			priqueueElement_t *temp=current->next->next;
			free(current->next);
			current->next=temp;

			removed++;
		}
		else
		{
			current=current->next;
		}
	}


	q->size-=removed;
	return removed;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
	void *rv=NULL;		//NOTE: could inefficently use priqueue_at
	int i;


	if(0<=index && index<q->size)
	{
		priqueueElement_t *current=q->head;
		for(i=0; i<index; i++)
		{
			current=current->next;
		}
		rv=current->next->data;

		priqueueElement_t *temp=current->next->next;
		free(current->next);
		current->next=temp;

		q->size--;
	}


	return rv;
}


/**
  Returns the number of elements in the queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->size;
}


/**
  Destroys and frees all the memory associated with q.

  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	while(q->head->next!=NULL)
	{
		priqueueElement_t *temp=q->head->next->next;
		free(q->head->next);
		q->head->next=temp;
	}
	free(q->head);


	return;
}


