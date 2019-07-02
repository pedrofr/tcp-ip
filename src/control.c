#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "error.h"
#include "control.h"
#include "controller.h"
#include "comm_consts.h"
#include "time_utils.h"
#include "terminal_utilities.h"

void *control(void *args)
{
	timestamp_printf("Starting control!\n");
	
	pararg *parg = (pararg *)args;
	parscomm *pcomm = parg->pcomm;
	contpar cpar = {50, 50, 0};
	struct timespec sleepTime = {0, 5000000L};

	pthread_t controller_thread;

	int errnum;
	if ((errnum = pthread_create(&controller_thread, NULL, controller, NULL)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	while(loading_controller());

	pthread_t keyboard_thread;
	if ((errnum = pthread_create(&keyboard_thread, NULL, keyboard_handler, parg)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	request_ownership(parg, CONTROL);
	while (!matches_arg(pcomm->command, pcomm->argument, "Start", OK))
	{
		strcpy(pcomm->command, "Start");
		strcpy(pcomm->argument, "");

		wait_for_response(parg, CLIENT, CONTROL);
	}

	while (!matches_arg(pcomm->command, pcomm->argument, "Exit", OK))
	{
		strcpy(pcomm->command, "GetLevel");
		strcpy(pcomm->argument, "");

		wait_for_response(parg, CLIENT, CONTROL);

		if (matches_numeric(pcomm->command, pcomm->argument, "Level"))
		{
			cpar.level = atof(pcomm->argument);
		}
		else if (matches_arg(pcomm->command, pcomm->argument, "Exit", OK))
		{
			break;
		}

		update_controller(&cpar);
		
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

		wait_for_response(parg, CLIENT, CONTROL);

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
			break;
		}

		update_controller(&cpar);
		
		grant_ownership(parg, CONTROL | TERMINAL);

		nanosleep(&sleepTime, NULL);

		request_ownership(parg, CONTROL);
	}

	quit_controller();
	pthread_join(controller_thread, NULL);

	quit_keyboard_handler();
	grant_ownership(parg, TERMINAL);
	pthread_join(keyboard_thread, NULL);

	grant_ownership(parg, CLIENT);

	timestamp_printf("Closing control!\n");

	pthread_exit(NULL);
}