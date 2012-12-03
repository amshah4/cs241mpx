/** @file parmake.c */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>


//globals
queue_t *rules;
queue_t *rulesDup;
queue_t *nextRule;
sem_t ruleAvailable;
sem_t threadAvailable;
pthread_mutex_t lock;



//function prototypes
void runRule(void *ptr);
void* parsed_new_target(char *target);
void* parsed_new_dependency(char *target, char *dependency);
void* parsed_new_command(char *target, char *command);
int findTarget(char *target);

//just incase things, to avoid silly bugs
typedef struct _result_t{}result_t;


/**
 * Entry point to parmake.
 */
int main(int argc, char **argv)
{
	int threads=1;						//working threads, additional to main thread
	char *fname=0;						//makefile name
	char temp=0;						//used for whatever
	char **targets=NULL;				//targets array
	int targetsGiven=0;					//targets count
	queue_init(rules);					//queue initialized
	queue_init(nextRule);				//queue initialized
	queue_init(rulesDup);				//queue initialized
	pthread_t *tid=NULL;				//array of thread IDs
	int i;								//loop iterator variable
	result_t *dummy=NULL;					//dummy variable, just in case
	sem_init(&ruleAvailable, 0, 0);		//semaphore initialized
	lock=PTHREAD_MUTEX_INITIALIZER;		//mutex initialized
	/*threadAvailable*/					//semaphore initialized (later)


	//part1
	while ((temp = getopt(argc, argv, "f:j:")) != -1)
	{
		if(temp=='f')
		{
			fname=optarg;
		}
		else if(temp=='j')
		{
			if(optarg!=0)
				threads=atoi(optarg);
		}
		else	//Use of option other than f and j
		{
			return -1;	//unspecified, so I just did same thing as no make file
		}				//I hate having multiple returns, but its faster for now
	}
	if(fname==0)
	{
		if(access("./makefile", F_OK)==0)
			fname="./makefile";
		else if(access("./Makefile", F_OK)==0)
			fname="./Makefile";
		else
			return -1;	//I hate having multiple returns, but its faster for now
	}
	targetsGiven=argc-optind;
	if(targetsGiven)
	{
		targets=malloc((targetsGiven+1)*sizeof(char*));
		for(i=0; i<targetsGiven; i++)
		{
			targets[i]=argv[optind+i];
		}
		targets[targetsGiven]=NULL;
	}


	//part 2
	parser_parse_makefile(fname, targets, 	parsed_new_target,
											parsed_new_dependency,
											parsed_new_command);


	//part 3 and 4
	tid=malloc(threads*sizeof(pthread_t));
	sem_init(&threadAvailable, 0, threads);

	for(i=0; i<threads; i++)
		pthread_create(&tid[i], NULL, runRule, NULL);

	while(queue_size(rules))
	{
		i=-1;
		int found=0;
		rule_t *temp=NULL;
		while(i<numRules && !found)//numRules is a formality, should always be found
		{
			i++;
			temp=queue_at(q, i);
			found=1;
			if(queue_size(temp->deps)!=0)
			{
				int j;
				int k;
				for(j=0; j<queue_size(rulesDup); j++)
				{
					for(k=0; k<queue_size(temp->deps); k++)
					{
						if(strcmp(queue_at(temp->deps, k), queue_at(rulesDup, j)->target)==0)
						{
							found=0;
						}
					}
				}
			}
		}


		sem_wait(&threadAvailable);

		pthread_mutex_lock(&lock);
		//CRITICAL SECTION
		queue_remove_at(rules, i);
		queue_enqueue(nextRule, temp);
		//END CRITICAL SECTION
		pthread_mutex_unlock(&lock);

		sem_post(&ruleAvailable);
	}

	//set up kickers for threads stuck in wait. Won't always be needed
	for(i=0; i<threads; i++)
	{
		pthread_mutex_lock(&lock);
		//CRITICAL SECTION
		queue_enqueue(nextRule, NULL);
		//END CRITICAL SECTION
		pthread_mutex_unlock(&lock);

		sem_post(&ruleAvailable);//waking up waiting threads to kick them
	}
	//wait for threads to finish
	for(i=0; i<threads; i++)
	{
		pthread_join(tid[i], (void**)&dummy);
	}


	while(queue_size(rulesDup))
	{
		rule_t *temp=queue_dequeue(rulesDup);

		rule_destroy(temp);
		free(temp);
	}
	queue_destroy(rules);
	queue_destroy(nextRule);
	sem_destroy(&threadAvailable);
	sem_destroy(&ruleAvailable);
	pthread_mutex_destroy(&lock);
	if(targetsGiven)
	{
		free(targets);
	}
	free(tid);
	return 0;
}


/* creates a target and adds to queue.
 *
 * @param - string name of target
 * @return - nothing? function type still requires the void pointer
 */
void* parsed_new_target(char *target)
{
	rule_t *temp=malloc(sizeof(rule_t));
	rule_init(temp);
	temp->target=target;
	queue_enqueue(rules, temp);
	queue_enqueue(rulesDup, temp);


	return NULL;
}

/* finds proper rule object and adds the appropriate attribute. The parser
 * handles calling parsed_new_target() for the dependencies.
 *
 * @param - string name of target and its dependency to add
 * @return - nothing? function type still requires the void pointer
 */
void* parsed_new_dependency(char *target, char *dependency)
{
	queue_enqueue(queue_at(rules, findTarget(target, rules))->deps, dependency);


	return NULL;
}

/* finds proper rule object and adds the appropriate attribute.
 *
 * @param - string name of target and its command to add
 * @return - nothing? function type still requires the void pointer
 */
void* parsed_new_command(char *target, char *command)
{
	queue_enqueue(queue_at(rules, findTarget(target, rules))->commands, command);


	return NULL;
}

//thread function
void* runRule(void *ptr)
{
	int kicked=0;
	int dontRun=0;
	rule_t *rule=NULL;


	while(queue_size(rules) && queue_size(nextRule) && !kicked)
	{
		sem_wait(&ruleAvailable);

		pthread_mutex_lock(&lock);
		//CRITICAL SECTION
		rule=queue_dequeue(nextRule);
		//END CRITICAL SECTION
		pthread_mutex_unlock(&lock);
		if(rule!=NULL)
		{
			int i=0;
			while(i<queue_size(rule->deps) && !dontRun)
			{
				char *dep=queue_dequeue(rule->deps);
				if(access(dep, F_OK)==0)
				{
					stat *stats;
					stat(dep, stats);
					time_t thisMod=stats->st_mtime;

					dontRun=1;
					int k;
					for(k=0; k<queue_size(rule->deps); k++)
					{

						time_t otherMod=stats->st_mtime;
						if(otherMod>thisMod)
						{
							dontRun=0;
						}
					}
				}


				i++;
			}
			if(!dontRun)
			{
				while(queue_size(rule->commands))
				{
					if(system(queue_dequeue(rule->commands))!=0)
					{
						exit(1);
					}
				}
			}
			pthread_mutex_lock(&lock);
			//CRITICAL SECTION
			for(i=0; i<queue_size(rulesDup); i++)
			{
				queue_t *temp=queue_at(rulesDup, i)->deps;
				int k;
				for(k=0; k<queue_size(temp); k++)
				{
					if(strcmp(queue_at(temp, k), rule->target)==0)
					{
						queue_remove_at(temp, k);
					}
				}

			}
			//END CRITICAL SECTION
			pthread_mutex_unlock(&lock);
		}
		else
		{
			kicked=1;
		}

		sem_post(&threadAvailable);
	}


	return NULL;
}


/* find index of rule matching target
 *
 * @param - string target
 * @return - index of corresponding rule
 */
int findTarget(char *target, queue_t *q)
{
	int numRules=queue_size(q);
	int i=-1;
	int found=0;
	rule_t *temp=NULL;


	while(i<numRules && !found)//numRules is a formality, should always be found
	{
		i++;
		temp=queue_at(q, i);
		if(strcmp(temp->target, target)==0)
		{
			found=1;
		}
	}


	return i;
}
