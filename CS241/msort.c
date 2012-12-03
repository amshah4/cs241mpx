/** @file msort.c */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


//Prototypes
void* merge(void *args);
void* sortSeg(void *args);
int intCmp(const void *x, const void *y);
int parse(char *x);


//Globals (ugh)
int *values;
int size;


/**
 * Data structure to store ranges of values for each segment
 */
typedef struct _Range
{
	int start, start2, end, numValues;
} Range;


/**
 * Dummy return type for pthread function, don't know if needed.
 */
typedef struct _Result
{

} Result;


/* Ankoor Shah
 * MP4 CS241
 *
 * This program carries out a merge sort alogoithm it is run by another
 * program that generates the array, and runs this program with the array
 * being fed to to stdin
 */
int main(int argc, char **argv)
{
	if(argc>1)
	{
		values=NULL;
		size=0;
		int segCount=parse(argv[1]);//apparently everyone besides me knew that parsing was required.. How else do we get the segCount int...
		int i=0;
		int numPerSeg=0;
		pthread_t *tid=malloc(segCount*sizeof(pthread_t));
		Range *ranges=malloc(segCount*sizeof(Range));


		//get all the values
		while(scanf("%d", &i)!=EOF)
		{
			size++;
			values=realloc(values, size*sizeof(int));
			values[size-1]=i;
		}

		if(segCount<=size)
		{
			numPerSeg=size/segCount;
			if(size%segCount!=0)
				numPerSeg++;


			//begin sorting segments
			for(i=0; i<segCount-1; i++)
			{
				ranges[i].start=i*numPerSeg;
				ranges[i].end=(i+1)*numPerSeg-1;
				ranges[i].numValues=numPerSeg;
				pthread_create(&tid[i], NULL, sortSeg, &ranges[i]);
			}
			ranges[i].start=i*numPerSeg;
			ranges[i].end=size-1;
			ranges[i].numValues=ranges[i].end-ranges[i].start+1;
			pthread_create(&tid[i], NULL, sortSeg, &ranges[i]);

			Result *temp;
			for(i=0; i<segCount; i++)
				pthread_join(tid[i], (void**)&temp);
			free(tid);


			//begin merging
			while(segCount>1)
			{
				tid=malloc((segCount/2)*sizeof(pthread_t));
				for(i=1; i<segCount; i+=2)
				{
					ranges[i/2].start=ranges[i-1].start;
					ranges[i/2].start2=ranges[i].start;
					ranges[i/2].end=ranges[i].end;
					ranges[i/2].numValues=ranges[i/2].end-ranges[i/2].start+1;
					pthread_create(&tid[i/2], NULL, merge, ranges[i/2]);
				}
				if(segCount%2!=0 && segCount>4)
				{
					ranges[i/2].start=ranges[segCount-1].start;
					ranges[i/2].end=ranges[segCount-1].end;
					ranges[i/2].numValues=ranges[segCount-1].numValues;
				}


				for(i=0; i<segCount/2;i++)
					pthread_join(tid[i], (void**)&temp);

				if(segCount%2==0)
					segCount/=2;
				else
					segCount=segCount/2+1;

				free(tid);
				ranges=realloc(ranges, sizeof(Range*segCount));
			}
		}


		free(ranges);
		free(values);
	}


	return 0;
}


/*sorting function ran in threads
 *
 * @param - start index, end index, and number of elements attached to thread
 * 			 during creation. contained in Range type pointer
 * @return - elements in range are sorted least to greatest
 */
void* sortSeg(void *args)
{
	Range *temp=(Range*)args;
	Result *dummy=NULL;


	qsort(&values[temp->start], temp->numValues, sizeof(int), intCmp);
	fprintf(stderr, "Sorted %d elements.\n", temp->numValues);


	return dummy;
}

//needed for sortSeg's use of qsort
int intCmp(const void *x, const void *y)
{
	return *((const int *)y) - *((const int *)x);
}

//needed to parse input string to int
int parse(char *x)
{
	int i;
	int end=strlen(x);
	int rv=0;
	for(i=0; i<end; i++)
	{
		int i2, pow=1;
		for(i2=0; i2<i; i2++)
			pow*=10;

		rv+=pow*(x[end-1-i]-48);
	}


	return rv;
}

/* Merge function ran in threads to complete merge sort. Should run in O(n)
 *
 * @param - args sent by thread, should contain start and end
 * @return - sections representing segments are sorted together
 */
void* merge(void *args)
{
	Range *temp=(Range*)args;
	int sorted[temp->numValues];
	int dupes=0;


	int i=temp->start, j=0, k=temp->start2;
	while(i<temp->start2 && k<=temp->end)
	{
		if(values[i]==values[k])
		{
			dupes++;
			sorted[j]=values[i];
			i++;
		}
		else if(values[i]<values[k])
		{
			sorted[j]=values[i];
			i++;
		}
		else
		{
			sorted[j]=values[k];
			k++;
		}
		j++;
	}

	//For case when segments are uneven
	while(i<temp->start2)
	{
		sorted[j]=values[i];
		i++;
		j++;
	}
	//The following should never run, but just in case...
	while(k<=temp->end)
	{
		sorted[j]=values[k];
		k++;
		j++;
	}

	//copy array over
	for(j=0; j<temp->numValues; j++)
	{
		values[temp->start+j]=sorted[j];
	}
}


