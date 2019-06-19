#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "error.h"
#include "simulator.h"
#include "comm_consts.h"
#include "plant.h"

void wait_request(pararg *parg, int id);
void release(pararg *parg, int id);

void *simulate(void *args)
{
	pararg *parg = (pararg *)args;

	parscomm *pcomm = parg->pcomm;

	char buffer_out[256];
	int errnum;

	printf("\nStarting simulator!\n");

	int plant_running = 0;

	int leave = 0;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	plantpar ppar = {0., 100., 0., 0, &mutex};

	pthread_t plant_thread;

	while (1)
	{
		wait_request(parg, SIMULATOR);

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
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Start"))
		{
			if (!plant_running)
			{
				if ((errnum = pthread_create(&plant_thread, NULL, plant, &ppar)))
				{
					sprintf(buffer_out, "Thread creation failed: %d\n", errnum);
					error(buffer_out);
				}

				plant_running = 1;
			}
			
			strcpy(pcomm->command, "Start");
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

		release(parg, SIMULATOR);

		if (leave)
			break;
	}

	pthread_join(plant_thread, NULL);

	printf("\nClosing simulator!\n");

	pthread_exit(NULL);
}

void wait_response(pararg *parg, int id)
{
	release(parg, id);
	wait_request(parg, id);
}

void wait_request(pararg *parg, int id)
{
	pthread_mutex_lock(parg->mutex);
	while (parg->holder == id)
		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
}

void release(pararg *parg, int id)
{
	parg->holder = id;
	pthread_mutex_unlock(parg->mutex);
	pthread_cond_signal(parg->cond);
}
