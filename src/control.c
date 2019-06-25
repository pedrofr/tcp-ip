#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "error.h"
#include "control.h"
#include "comm_consts.h"
#include "plant.h"

typedef plantpar contpar;

void *control(void *args)
{
	pararg *parg = (pararg *)args;

	parscomm *pcomm = parg->pcomm;

	printf("\nStarting control!\n");

	int leave = 0;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	contpar cpar = {0., 100., 0., 0, &mutex};

	pthread_t controller_thread;

	int errnum;
	if ((errnum = pthread_create(&controller_thread, NULL, controller, &cpar)))
	  {
	    char buffer_out[256];
	    sprintf(buffer_out, "Thread creation failed: %d\n", errnum);
	    error(buffer_out);
	  }

	while (1)
	{
		wait_request(parg, CONTROL);

		if (matches_numeric(pcomm->command, pcomm->argument, "OpenValve"))
		{
			double value = atof(pcomm->argument);

			pthread_mutex_lock(&mutex);
			ppar.delta += value;
			pthread_mutex_unlock(&mutex);

			sprintf(pcomm->argument, "%i", (int)value);
			strcpy(pcomm->command, "Open");
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "CloseValve"))
		{
			double value = atof(pcomm->argument);

			pthread_mutex_lock(&mutex);
			ppar.delta -= value;
			pthread_mutex_unlock(&mutex);

			sprintf(pcomm->argument, "%i", (int)value);
			strcpy(pcomm->command, "Close");
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "SetMax"))
		{
			double value = atof(pcomm->argument);

			pthread_mutex_lock(&mutex);
			ppar.max = value;
			pthread_mutex_unlock(&mutex);

			sprintf(pcomm->argument, "%i", (int)value);
			strcpy(pcomm->command, "max");
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "GetLevel"))
		{
			int level;

			pthread_mutex_lock(&mutex);
			level = (int)(ppar.level*100);
			pthread_mutex_unlock(&mutex);

			sprintf(pcomm->argument, "%i", level);
			strcpy(pcomm->command, "Level");
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "CommTest"))
		{
			strcpy(pcomm->command, "Comm");
			strcpy(pcomm->argument, OK);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Exit"))
		{
			strcpy(pcomm->command, "Exit");
			strcpy(pcomm->argument, OK);

			pthread_mutex_lock(&mutex);
			ppar.leave = leave = 1;
			pthread_mutex_unlock(&mutex);
		}
		else
		{
			strcpy(pcomm->command, "NoMessageFound!");
			strcpy(pcomm->argument, "");
		}

		release(parg, CONTROL);

		if (leave)
			break;
	}

	pthread_join(controller_thread, NULL);

	printf("\nClosing control!\n");

	pthread_exit(NULL);
}
