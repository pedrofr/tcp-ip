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

void *simulate(void *args)
{
	timestamp_printf("Starting simulator!");

	pararg *parg = (pararg *)args;
	parscomm *pcomm = parg->pcomm;

	int plant_running = 0;
	int leave = 0;

	pthread_t plant_thread;

	while (1)
	{
		wait_request(parg, SIMULATOR);

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
		else if (matches_no_arg(pcomm->command, pcomm->argument, "CommTest"))
		{
			strcpy(pcomm->command, "Comm");
			strcpy(pcomm->argument, OK);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Start"))
		{
			if (!plant_running)
			{
				int errnum;
				if ((errnum = pthread_create(&plant_thread, NULL, plant, NULL)))
				{
					errorf("\nThread creation failed: %d\n", errnum);
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

			leave = 1;
		}
		// else
		// {
		// 	strcpy(pcomm->command, "NoMessageFound");
		// 	strcpy(pcomm->argument, "");
		// }

		release(parg, SIMULATOR);

		if (leave)
			break;
	}

	quit_plant();
	pthread_join(plant_thread, NULL);

	timestamp_printf("Closing simulator!");

	pthread_exit(NULL);
}
