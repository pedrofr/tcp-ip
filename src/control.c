#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "error.h"
#include "control.h"
#include "controller.h"
#include "comm_consts.h"

void *control(void *args)
{
	pararg *parg = (pararg *)args;

	parscomm *pcomm = parg->pcomm;

	printf("\nStarting control!\n");

	int leave = 0;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	contpar cpar = {50, 50, 0, 0, &mutex};

	pthread_t controller_thread;

	struct timespec sleepTime = {0, 10000000L};

	int errnum;
	if ((errnum = pthread_create(&controller_thread, NULL, controller, &cpar)))
	{
		char buffer_out[256];
		sprintf(buffer_out, "Thread creation failed: %d\n", errnum);
		error(buffer_out);
	}

	while (!matches_arg(pcomm->command, pcomm->argument, "Start", OK))
	{
		strcpy(pcomm->command, "Start");
		strcpy(pcomm->argument, "");

		wait_response(parg, CONTROL);
	}

	while (1)
	{
		strcpy(pcomm->command, "GetLevel");
		strcpy(pcomm->argument, "");

		wait_response(parg, CONTROL);

		if (matches_numeric(pcomm->command, pcomm->argument, "Level"))
		{
			double value = atof(pcomm->argument);

			pthread_mutex_lock(&mutex);
			cpar.level = value;
			pthread_mutex_unlock(&mutex);
		}
		else if (matches_arg(pcomm->command, pcomm->argument, "Exit", OK))
		{
			pthread_mutex_lock(&mutex);
			cpar.leave = leave = 1;
			pthread_mutex_unlock(&mutex);
		}

		pthread_mutex_lock(&mutex);
		int angle_diff = cpar.requested_angle - cpar.reported_angle;
		pthread_mutex_unlock(&mutex);

    	// printf("\ncpar.requested_angle: '%i'\ncpar.reported_angle '%i", cpar.requested_angle, cpar.reported_angle);

		if (angle_diff > 0)
		{	
			strcpy(pcomm->command, "OpenValve");
			sprintf(pcomm->argument, "%i", angle_diff);
		}
		else if (angle_diff < 0)
		{
			strcpy(pcomm->command, "CloseValve");
			sprintf(pcomm->argument, "%i", -angle_diff);
		}

		wait_response(parg, CONTROL);

		if (matches_numeric(pcomm->command, pcomm->argument, "Open"))
		{
			double value = atof(pcomm->argument);

			pthread_mutex_lock(&mutex);
			cpar.reported_angle += value;
			pthread_mutex_unlock(&mutex);
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "Close"))
		{
			double value = atof(pcomm->argument);

			pthread_mutex_lock(&mutex);
			cpar.reported_angle -= value;
			pthread_mutex_unlock(&mutex);
		}
		else if (matches_arg(pcomm->command, pcomm->argument, "Exit", OK))
		{
			pthread_mutex_lock(&mutex);
			cpar.leave = leave = 1;
			pthread_mutex_unlock(&mutex);
		}

		if (leave) {
			release(parg, CONTROL);
			break;
		}

		nanosleep(&sleepTime, NULL);
	}

	pthread_join(controller_thread, NULL);

	printf("\nClosing control!\n");

	pthread_exit(NULL);
}
