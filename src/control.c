#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "error.h"
#include "control.h"
#include "controller.h"
#include "comm_consts.h"
#include "time_utils.h"
#include "control_utils.h"
#include "terminal_utilities.h"
#include "graphics.h"

#include <math.h>

static volatile sig_atomic_t load = 1;

void *control(void *args)
{
	timestamp_printf("Starting control!\n");
	
	pararg *parg = (pararg *)args;
	parscomm *pcomm = parg->pcomm;
	contpar cpar = {INITIAL_ANGLE, INITIAL_LEVEL};
	int estimated_angle = INITIAL_ANGLE;

	pthread_t controller_thread, graph_thread;

	int errnum;
	if ((errnum = pthread_create(&graph_thread, NULL, graphics, NULL)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	request_ownership(parg, CONTROL);
	load = 0;

	while (!matches_arg(pcomm->command, pcomm->argument, "Max", "100"))
	{
		strcpy(pcomm->command, "SetMax");
		strcpy(pcomm->argument, "100");

		wait_for_response(parg, CONTROL, CLIENT);
	}

	while (loading_graphics());

	if ((errnum = pthread_create(&controller_thread, NULL, controller, NULL)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	while (loading_controller());

	while (!matches_arg(pcomm->command, pcomm->argument, "Start", OK))
	{
		strcpy(pcomm->command, "Start");
		empty(pcomm->argument);

		wait_for_response(parg, CONTROL, CLIENT);
	}

	pthread_t keyboard_thread;
	if ((errnum = pthread_create(&keyboard_thread, NULL, keyboard_handler, parg)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	while (strcmp(pcomm->command, "Exit"))
	{
		strcpy(pcomm->command, "GetLevel");
		empty(pcomm->argument);

		wait_for_response(parg, CONTROL, CLIENT);

		if (matches_numeric(pcomm->command, pcomm->argument, "Level"))
		{
			cpar.level = atof(pcomm->argument);
		}
		else if (!strcmp(pcomm->command, "Exit"))
		{
			break;
		}

		update_controller(&cpar);

		update_graphics(cpar.level, cpar.angle, 0);
		
		int next_send = cpar.angle - estimated_angle;
		estimated_angle = cpar.angle;

		if (next_send > 0)
		{	
			strcpy(pcomm->command, "OpenValve");
			sprintf(pcomm->argument, "%i", next_send);
		}
		else if (next_send < 0)
		{
			strcpy(pcomm->command, "CloseValve");
			sprintf(pcomm->argument, "%i", -next_send);
		}

		wait_for_response(parg, CONTROL, CLIENT);

		if (!strcmp(pcomm->command, "Exit"))
		{
			break;
		}

		update_controller(&cpar);

		update_graphics(cpar.level, cpar.angle, 0);
		
		grant_ownership(parg, CONTROL, CONTROL | TERMINAL);

		request_ownership(parg, CONTROL);
	}

	quit_keyboard_handler();
	grant_ownership(parg, CONTROL, TERMINAL);
	quit_controller();
	quit_graphics();

	pthread_join(controller_thread, NULL);
	pthread_join(keyboard_thread, NULL);
	pthread_join(graph_thread, NULL);

	release_ownership(parg, CONTROL);

	timestamp_printf("Closing control!\n");

	pthread_exit(NULL);
}

sig_atomic_t loading_control()
{
	return load;
}
