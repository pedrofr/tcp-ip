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

static volatile sig_atomic_t quit;
static volatile sig_atomic_t load = 1;

void *control(void *args)
{
	timestamp_printf("Starting control!\n");
	
	pararg *parg = (pararg *)args;
	parscomm *pcomm = parg->pcomm;
	int previous_angle = INITIAL_ANGLE, level = INITIAL_LEVEL;

	double reference = REFERENCE;
	struct timespec time_start, time_last, time_current;

	now(&time_start);
	time_last = time_current = time_start;
	perspec pspec = {time_start, CONTROLLER_PERIOD};

	pthread_t graph_thread;

	int errnum;
	if ((errnum = pthread_create(&graph_thread, NULL, graphics, NULL)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	request_ownership(parg, CONTROL);
	load = 0;

  	int try = 0;
	while (!matches_arg(pcomm->command, pcomm->argument, "Max", "100"))
	{
    	timestamp_force_printf("'SetMax#100!' try #%i", ++try);

		strcpy(pcomm->command, "SetMax");
		strcpy(pcomm->argument, "100");

		wait_for_response(parg, CONTROL, CLIENT);
	}

	while (loading_graphics());

  	try = 0;
	while (!matches_arg(pcomm->command, pcomm->argument, "Start", OK))
	{
    	timestamp_force_printf("'Start!' try #%i", ++try);
		strcpy(pcomm->command, "Start");
		empty(pcomm->argument);

		wait_for_response(parg, CONTROL, CLIENT);
	}

	while (!quit && strcmp(pcomm->command, "Exit"))
	{
		strcpy(pcomm->command, "GetLevel");
		empty(pcomm->argument);

		wait_for_response(parg, CONTROL, CLIENT);

		if (matches_numeric(pcomm->command, pcomm->argument, "Level"))
		{
			level = atof(pcomm->argument);
		}
		else if (!strcmp(pcomm->command, "Exit"))
		{
			break;
		}
		
		now(&time_current);
		// double T = timediff(&time_current, &time_start);
		double dT = timediff(&time_current, &time_last);
		time_last = time_current;

		// timestamp_printf("T: %11.4f | dT: %7.4f", T, dT);

		int angle = pid(dT, level, reference);

		// printf(" | angle: %i\n", angle);

		update_graphics(level, angle, 0);
		
		int next_send = angle - previous_angle;
		previous_angle = angle;

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
		else
		{
			ensure_period(&pspec);
			continue;
		}

		wait_for_response(parg, CONTROL, CLIENT);

		if (!strcmp(pcomm->command, "Exit"))
		{
			break;
		}

		ensure_period(&pspec);
	}

	quit_graphics();
	pthread_join(graph_thread, NULL);

	release_ownership(parg, CONTROL);

	timestamp_printf("Closing control!\n");

	pthread_exit(NULL);
}

sig_atomic_t loading_control()
{
	return load;
}

void quit_control()
{
	quit = 1;
}
