#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "error.h"
#include "simulator.h"
#include "comm_consts.h"
#include "plant.h"
#include "time_utils.h"

static volatile sig_atomic_t quit;

void *simulate(void *args)
{
	timestamp_printf("Starting simulator!\n");

	pararg *parg = (pararg *)args;
	parscomm *pcomm = parg->pcomm;

	unsigned char plant_running = 0;

	pthread_t plant_thread;

	while (!quit)
	{
		request_ownership(parg, SIMULATOR);

		if (matches_numeric(pcomm->command, pcomm->argument, "OpenValve"))
		{
			int value = atoi(pcomm->argument);
			update_delta(value);

			strcpy(pcomm->command, "Open");
			sprintf(pcomm->argument, "%i", value);
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "CloseValve"))
		{
			int value = atoi(pcomm->argument);
			update_delta(-value);

			strcpy(pcomm->command, "Close");
			sprintf(pcomm->argument, "%i", value);
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "SetMax"))
		{
			int value = atoi(pcomm->argument);
			update_max(value);

			strcpy(pcomm->command, "Max");
			sprintf(pcomm->argument, "%i", value);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "GetLevel"))
		{
			int level;
			read_level(&level);

			strcpy(pcomm->command, "Level");
			sprintf(pcomm->argument, "%i", level);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Start"))
		{
			if (plant_running)
			{
				restart_plant();
				while (loading_plant());				
			}
			else
			{
				int errnum;
				if ((errnum = pthread_create(&plant_thread, NULL, plant, NULL)))
				{
					errorf("\nThread creation failed: %d\n", errnum);
				}

				while (loading_plant());
				plant_running = 1;
			}

			strcpy(pcomm->command, "Start");
			strcpy(pcomm->argument, OK);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Exit"))
		{
			strcpy(pcomm->command, "Exit");
			strcpy(pcomm->argument, OK);

			quit = 1;
		}

		grant_ownership(parg, SIMULATOR, SERVER);
	}
	
	quit_plant();
	pthread_join(plant_thread, NULL);

	timestamp_printf("Closing simulator!\n");

	pthread_exit(NULL);
}

void quit_simulator()
{
	quit = 1;
}
