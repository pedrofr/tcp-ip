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

	contpar cpar = {50, 50, 0};

	pthread_t controller_thread;

	struct timespec sleepTime = {0, 5000000L};

	int errnum;
	if ((errnum = pthread_create(&controller_thread, NULL, controller, NULL)))
	{
		char buffer_out[BUFFER_SIZE];
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
			cpar.level = atof(pcomm->argument);
		}
		else if (matches_arg(pcomm->command, pcomm->argument, "Exit", OK))
		{
			release(parg, CONTROL);
			break;
		}

		update_controller(cpar);
		read_controller(&cpar);
		
		int angle_diff = cpar.requested_angle - cpar.reported_angle;

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
			cpar.reported_angle += atof(pcomm->argument);
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "Close"))
		{
			cpar.reported_angle -= atof(pcomm->argument);
		}
		else if (matches_arg(pcomm->command, pcomm->argument, "Exit", OK))
		{
			release(parg, CONTROL);
			break;
		}

		update_controller(cpar);

		nanosleep(&sleepTime, NULL);
	}

	quit_controller();
	pthread_join(controller_thread, NULL);

	printf("\nClosing control!\n");

	pthread_exit(NULL);
}
