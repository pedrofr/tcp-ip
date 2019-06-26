#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <stddef.h>
#include "error.h"
#include "plant.h"
#include "graph.h"
#include "control_utilities.h"
#include "controller.h"

void *controller(void *args)
{
	contpar *cpar = (contpar *)args;
	struct timespec time_initial, time_current;
	// struct timespec time_last;
	

	clock_gettime(CLOCK_MONOTONIC_RAW, &time_initial);
	time_current = time_initial;
	// time_last = time_current;

	char buffer[26];
	time_t timer;
	time(&timer);
	struct tm* tm_info = localtime(&timer);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("\nStarting controller at %s!\n", buffer);

	struct timespec sleepTime = {0, 10000000L};

	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	graphpar gpar = {0., 0., 0., 0., 0, &mutex};

	pthread_t graph_thread;

	int errnum;
	if ((errnum = pthread_create(&graph_thread, NULL, graph, &gpar)))
	{
		char buffer_out[256];
		sprintf(buffer_out, "Thread creation failed: %d\n", errnum);
		error(buffer_out);
	}

	double angle;

	while (1)
	{
		pthread_mutex_lock(cpar->mutex);
		int leave = cpar->leave;
		double level = cpar->level;
		double reported_angle = cpar->reported_angle;
		pthread_mutex_unlock(cpar->mutex);

		clock_gettime(CLOCK_MONOTONIC_RAW, &time_current);
		double T = (time_current.tv_sec - time_initial.tv_sec) * 1000. + (time_current.tv_nsec - time_initial.tv_nsec) / 1000000.;
		// double dT = (time_current.tv_sec - time_last.tv_sec) * 1000. + (time_current.tv_nsec - time_last.tv_nsec) / 1000000.;
		// time_last = time_current;

		if (level < 80)
		{
			angle = 100;
		}
		else if (level > 80)
		{
			angle = 0;
		}

		//TODO: Calculos do controlador

		/* printf("\nT: %11.4f | dT: %7.4f", T, dT); */

		/* printf(" | delta: %9.4f | in_angle: %9.4f | out_angle: %9.4f | level: %7.4f | influx: %f | outflux %f", delta, in_angle, out_angle, level, influx, outflux); */

		pthread_mutex_lock(cpar->mutex);
		cpar->requested_angle = (int)round(angle);
		pthread_mutex_unlock(cpar->mutex);
		
		pthread_mutex_lock(&mutex);
		gpar.leave = leave;
		gpar.var1 = level;
		gpar.var2 = reported_angle;
		gpar.var3 = 0;
		gpar.time = T/1000;
		pthread_mutex_unlock(&mutex);

		if (leave)
			break;

		nanosleep(&sleepTime, NULL);
	}

	pthread_join(graph_thread, NULL);

	time(&timer);
	tm_info = localtime(&timer);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("\nClosing controller at %s!\n", buffer);

	pthread_exit(NULL);
}

