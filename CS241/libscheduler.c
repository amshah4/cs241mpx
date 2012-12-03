/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements.
*/
typedef struct _job_t
{
	int arrivalTime;
	int runningTime;
	int priority;
	int jobNum;

} job_t;

job_t *myCores;				//each int represents bool
int numCores;
priqueue_t *jobs;
scheme_t myScheme;



/**
  Initalizes the scheduler.

  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
	myCores=malloc(cores*sizeof(job_t));
	numCores=cores;
	jobs=malloc(sizeof(priqueue_t));
	myScheme=scheme;
	int i;
	for(i=0; i<cores; i++)
		myCores[i]=NULL;


	switch(scheme)
	{
		case FCFS:
			priqueue_init(jobs, comparerFCFS);
		break;
		case SJF:
			priqueue_init(jobs, comparerSJF);
		break;
		case PSJF:
			priqueue_init(jobs, comparerPSJF);
		break;
		case PRI:
			priqueue_init(jobs, comparerPRI);
		break;
		case PPRI:
			priqueue_init(jobs, comparerPPRI);
		break;
		case RR:
			priqueue_init(jobs, comparerRR);
		break;
		default:
			printf("\n\nSOMETHING IS TERRIBLY WRONG HERE\n\n");
		break;
	}
}


/**
  Called when a new job arrives.

  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made.

 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
	int i=-1;
	int index=-1;
	int found=0;
	job_t *newJob=malloc(sizeof(job_t));
	newJob->arrivalTime=time;
	newJob->jobNum=job_number;
	newJob->priority=priority;


	while(i<numCores && !found)
	{
		i++;

		if(myCores[i]==NULL)
		{
			myCores[i]=newJob;
			found=1;
		}
	}
	index=i;
	if(!found)
	{
		i=-1;
		index=-1;


		if(myScheme==PPRI)
		{
			int lowestPri=0;
			while(i<numCores)
			{
				i++;
				if(comparerPRI(myCores[lowestPri], myCores[i])<0)
				{
					lowestPri=i;
				}
			}
			if(comparerPRI(newJob, myCores[lowestPri])<0)
			{
				job_t *temp=myCores[lowestPri];
				myCores[lowestPri]=newJob;
				newJob=temp;
				newJob->runningTime=newJob->arrivalTime+newJob->runningTime-time;

				index=lowestPri;
			}
		}
		else if(myScheme==PSJF)
		{
			int longest=0;
			while(i<numCores)
			{
				i++;
				if(comparerSJF(myCores[longest], myCores[i])<0)
				{
					longest=i;
				}
			}
			if(comparerSJF(newJob, myCores[longest])<0)
			{
				job_t *temp=myCores[longest];
				myCores[longest]=newJob;
				newJob=temp;
				newJob->runningTime=newJob->arrivalTime+newJob->runningTime-time;

				index=longest;
			}
		}
		else
		{
			//do nothing
		}

		priqueue_offer(jobs, newJob);
	}


	return index;
}


/**
  Called when a job has completed execution.

  The core_id, job_number and time parameters are provided for convenience. You may
   	   	   	   	   	   be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
	free(myCores[core_id]);
	myCores[core_id]=NULL;
	job_t next=priqueue_poll(jobs);
	int rv=-1;
	if(next!=NULL)
	{
		scheduler_new_job(next->jobNum, time, next->runningTime, next->priority);
		rv=next->jobNum;
	}


	return rv;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.

  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	job_t *temp=myCores[core_id];
	myCores[core_id]=NULL;
	priqueue_offer(jobs, temp);
	job_t *next=priqueue_poll(jobs);
	int rv=-1;
	if(next!=NULL)//should always resolve to true
	{
		scheduler_new_job(next->jobNum, time, next->runningTime, next->priority);
		rv=next->jobNum;
	}


	return rv;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	return 0.0;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	return 0.0;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	return 0.0;
}


/**
  Free any memory associated with your scheduler.

  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	int i;
	for(i=0; i<numCores; i++)
		if(myCores[i]!=NULL)
			free(myCores[i]);

	while(jobs->size > 0)
		free(priqueue_poll(jobs));

	priqueue_destroy(jobs);


	return;
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)

  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{
	return;
}


//Comparer functions
int comparerFCFS(const void *x, const void *y)
{
	return 1;
}

int comparerSJF(const void *x, const void *y)
{
	int temp=((job_t*)x)->runningTime - ((job_t*)y)->runningTime;
	if(temp==0)
		temp=((job_t*)x)->arrivalTime - ((job_t*)y)->arrivalTime;


	return temp;
}

int comparerPSJF(const void *x, const void *y)
{
	return comparerSJF(x, y);
}

int comparerPRI(const void *x, const void *y)
{
	int temp=((job_t*)x)->priority - ((job_t*)y)->priority;
	if(temp==0)
		temp=((job_t*)x)->arrivalTime - ((job_t*)y)->arrivalTime;


	return temp;
}

int comparerPPRI(const void *x, const void *y)
{
	return comparerPRI(x, y);
}

int comparerRR(const void *x, const void *y)
{
	return 1;
}
