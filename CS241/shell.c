/** @file shell.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "log.h"

/**
 * Starting point for shell.
 */
int main()
{
	int done=0;
	size_t size=0;
	char *s=NULL;
	char *buf=NULL;
	int sLength=0;
	log_t *history=malloc(sizeof(log_t));
	log_init(history);
	char *temp=NULL;
	pid_t pid;
	int *status=NULL;
	char **args=NULL;


	while(!done)
	{
		printf("(pid=%d)%s$ ", getpid(), getcwd(buf, size));
		sLength=getline(&s, &size, stdin);
		s[sLength-1]='\0';

		temp=malloc(size);
		strcpy(temp, s);
		log_append(history, temp);
		if(strcmp(s, "exit")==0)
		{
			printf("Command executed by pid=%d\n", getpid());
			done=1;
		}
		else if(strncmp(s, "cd ", 3)==0)
		{
			printf("Command executed by pid=%d\n", getpid());
			if(chdir(s+3)==-1)
				printf("%s: No such file or directory\n", s+3);
		}
		else if(strcmp(s, "!#")==0)
		{
			int i;


			printf("Command executed by pid=%d\n", getpid());
			log_pop(history);

			for(i=0; i<log_size(history); i++)
			{
				printf("%s\n", log_at(history, i));
			}
		}
		else if(strncmp(s, "!", 1)==0)
		{
			log_pop(history);
			char *temp2=log_search(history, s+1);
			if(temp2!=NULL)
			{
				temp=malloc(strlen(temp2)+1);
				strcpy(temp, temp2);
				log_append(history, temp);

				if(fork()==0)
				{
					printf("Command executed by pid=%d\n", getpid());
					printf("%s matches %s\n", s+1, temp2);

					//PARSE
					char* tok=strtok(temp2, " ");
					int ind=0;
					while(tok!=NULL)
					{
						args[ind]=tok;
						tok=strtok(NULL, " ");
						ind++;
					}
					execvp(args[0], args);  //returns on error only

					printf("%s: not found\n", s+1);
					log_pop(history);		//Don't think it affects parent,
//it might thanks to pointers. If it doesn't I don't know how to signal parent
//that exec() failed
				}
				else
				{
					wait(status);
				}
			}
			else
			{
				printf("No Match\n");
			}
		}
		else
		{
			printf("Command executed by pid=%d\n", getpid());
			if(fork()==0)
			{
				//PARSE
				char* tok=strtok(temp2, " ");
				int ind=0;
				while(tok!=NULL)
				{
					args[ind]=tok;
					tok=strtok(NULL, " ");
					ind++;
				}
				execvp(args[0], args); //returns on error only

				printf("%s: not found\n", s);
			}
			else
			{
				wait(status);
			}
		}


		free(buf);
		free(s);
		free(status);
		buf=NULL;
		s=NULL;
		temp=NULL;
		status=NULL;
	}


	log_destroy(history);
    return 0;
}

