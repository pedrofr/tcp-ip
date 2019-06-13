#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "error.h"
#include "parse.h"
#include "simulator.h"
#include "comm_consts.h"

void wait_request(pararg *parg, int id);
void release(pararg *parg, int id);

void *simulate(void *args)
{
	pararg *parg = (pararg *)args;

	parscomm *pcomm = parg->pcomm;
	int *mod = parg->mod;

	char command[256];
	char argument[256];

	printf("\nStarting simulator!\n");

	double delta = 0;
	double Max = 100;
	double dt = 0.05;

	int leave = 0;

	while (1)
	{
		wait_request(parg, 1);

		if (matches_numeric(pcomm->command, pcomm->argument, "OpenValve"))
		{
			double value = atof(pcomm->argument);
			delta += value;
			sprintf(argument, "%i", (int)value);
			strcpy(pcomm->command, "Open");
			strcpy(pcomm->argument, argument);
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "CloseValve"))
		{
			double value = atof(pcomm->argument);
			delta -= value;
			sprintf(argument, "%i", (int)value);
			strcpy(pcomm->command, "Close");
			strcpy(pcomm->argument, argument);
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "SetMax"))
		{
			double value = atof(pcomm->argument);
			Max = value;
			sprintf(argument, "%i", (int)value);
			strcpy(pcomm->command, "Max");
			strcpy(pcomm->argument, argument);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "GetLevel"))
		{
			sprintf(argument, "%i", (int)delta);
			strcpy(pcomm->command, "Level");
			strcpy(pcomm->argument, argument);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "CommTest"))
		{
			strcpy(pcomm->command, "Comm");
			strcpy(pcomm->argument, OK);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Start"))
		{
			strcpy(pcomm->command, "Start");
			strcpy(pcomm->argument, OK);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Exit"))
		{
			strcpy(pcomm->command, "Exit");
			strcpy(pcomm->argument, OK);
			leave = 1;
		}
		else
		{
			strcpy(pcomm->command, "NoMessageFound!");
			strcpy(pcomm->argument, "");
		}

		release(parg, 1);

		if (leave)
			break;
	}

	printf("\nClosing simulator!\n");
}

void wait_response(pararg *parg, int id)
{
	release(parg, id);
	wait_request(parg, id);
}

void wait_request(pararg *parg, int id)
{
	pthread_mutex_lock(parg->mutex);
	while (*(parg->mod) == id)
		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
}

void release(pararg *parg, int id)
{
	*(parg->mod) = id;
	pthread_mutex_unlock(parg->mutex);
	pthread_cond_signal(parg->cond);
}
