/** @file alloc.c */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


//GLOBAL VARIABLES
typedef struct _MemoryBlock
{
	void *address;
	size_t size;
	int free; //bools aren't real, 0=false, 1=true
	struct _MemoryBlock *left;
	struct _MemoryBlock *right;
}MemoryBlock

MemoryBlock *memTable=NULL; // Dummy Node


/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size)
{
	/* Note: This function is complete. You do not need to modify it. */
	void *ptr = malloc(num * size);

	if (ptr)
		memset(ptr, 0x00, num * size);

	return ptr;
}


/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size)
{
	if(memTable==NULL)
	{
		memTable->address=NULL;
		memTable->free=0;
		memTable->left=NULL;
		memTable->right=NULL;
		memTable->size=0;
	}
	MemoryBlock *itr=memTable;		//iterator
	int done=0;
	void* addr=NULL;


	while(done==0)
	{
		if(size<itr->size)
		{
			if(itr->free)//reuse if available
				addr=itr->address;
			else if(itr->left==NULL)
				done=1;
			else
				itr=itr->left;
		}
		else
		{
			if(itr->right==NULL)
				done=2;
			else
				itr=itr->right;
		}
	}

	if(addr==NULL)
	{
		if(done==1)
		{
			itr->left=sbrk(sizeof(MemoryBlock));

			itr->left->address=sbrk(size);
			itr->left->free=1;
			itr->left->left=NULL;
			itr->left->right=NULL;
			itr->left->size=size;
		}
		else
		{
			itr->right=sbrk(sizeof(MemoryBlock));

			itr->right->address=sbrk(size);
			itr->right->free=1;
			itr->right->left=NULL;
			itr->right->right=NULL;
			itr->right->size=size;
		}
	}


	return addr;
}


/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr)
{
	// "If a null pointer is passed as argument, no action occurs."
	if (ptr!=NULL)//changed from original "if" because mult. returns is bad
	{
		free(ptr, memTable);
	}

	return;
}

int free(void *ptr, MemoryBlock* curr)
{
	int done=0;

	if(curr!=NULL)
	{
		if(ptr==curr->address)
		{
			curr->free=1;
			done=1;
		}
		else
		{
			if(!free(curr->left))
				free(curr->right);
		}
	}


	return done;
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size)
{
	void *addr=NULL;

	// "In case that ptr is NULL, the function behaves exactly as malloc()"
	if (ptr==NULL)
	{
		addr=malloc(size);
	}
	// "In case that the size is 0, the memory previously allocated in ptr
	//  is deallocated as if a call to free() was made, and a NULL pointer
	//  is returned."
	else if (size==0)
	{
		free(ptr);
	}
	else
	{
		addr=realloc(ptr, size, memTable);
	}


	return addr;
}

void* realloc(void *ptr, size_t size, MemoryBlock* curr)
{
	void *addr=NULL;

	if(curr!=NULL)
	{
		if(ptr==curr->address)
		{
			if(curr->size>=size)
			{
				addr=ptr;
			}
			else
			{
				addr=malloc(size);
				memcpy(addr, curr, curr->size);
				free(ptr);
			}

		}
		else
		{
			if(free(curr->left)==NULL)
				free(curr->right);
		}
	}


	return addr;
}
