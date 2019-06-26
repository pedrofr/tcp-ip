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

#define LEVEL_RATE 0.00002
#define VALVE_RATE 0.01

double out_angle_function(double time);

void *plant(void *args)
{
	plantpar *ppar = (plantpar *)args;
	double in_angle = 50;
	double level = 0.4;
	struct timespec time_initial, time_last, time_current;

	clock_gettime(CLOCK_MONOTONIC_RAW, &time_initial);
	time_current = time_initial;
	time_last = time_current;

	char buffer[26];
	time_t timer;
	time(&timer);
	struct tm *tm_info = localtime(&timer);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("\nStarting plant at %s!\n", buffer);

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

	while (1)
	{
		pthread_mutex_lock(ppar->mutex);
		int leave = ppar->leave;
		double delta_i = ppar->delta;
		double max = ppar->max;
		pthread_mutex_unlock(ppar->mutex);

		clock_gettime(CLOCK_MONOTONIC_RAW, &time_current);
		double T = (time_current.tv_sec - time_initial.tv_sec) * 1000. + (time_current.tv_nsec - time_initial.tv_nsec) / 1000000.;
		double dT = (time_current.tv_sec - time_last.tv_sec) * 1000. + (time_current.tv_nsec - time_last.tv_nsec) / 1000000.;
		time_last = time_current;

		double delta_f = delta_i;

		if (delta_f > 0)
		{
			if (delta_f < VALVE_RATE * dT)
			{
				in_angle += delta_f;
				delta_f = 0;
			}
			else
			{
				in_angle += VALVE_RATE * dT;
				delta_f -= VALVE_RATE * dT;
			}
		}
		else if (delta_f < 0)
		{
			if (delta_f > -VALVE_RATE * dT)
			{
				in_angle += delta_f;
				delta_f = 0;
			}
			else
			{
				in_angle -= VALVE_RATE * dT;
				delta_f += VALVE_RATE * dT;
			}
		}

		double influx = 1 * sin(M_PI / 2 * in_angle / 100);

		double out_angle = out_angle_function(T);
		double outflux = (max / 100) * (level / 1.25 + 0.2) * sin(M_PI / 2 * out_angle / 100);
		level += LEVEL_RATE * dT * (influx - outflux);

		//Saturação
		level = saturate(level, 0, 1, NULL);

		// printf("\nT: %11.4f | dT: %7.4f", T, dT);

		// printf(" | delta_i: %9.4f | in_angle: %9.4f | out_angle: %9.4f | level: %7.4f | influx: %f | outflux %f", delta_i, in_angle, out_angle, level, influx, outflux);

		pthread_mutex_lock(ppar->mutex);
		ppar->delta += delta_f - delta_i;
		ppar->level = level;
		pthread_mutex_unlock(ppar->mutex);

		pthread_mutex_lock(&mutex);
		gpar.leave = leave;
		gpar.var1 = level * 100;
		gpar.var2 = in_angle;
		gpar.var3 = out_angle;
		gpar.time = T / 1000;
		pthread_mutex_unlock(&mutex);

		if (leave)
			break;

		nanosleep(&sleepTime, NULL);
	}

	pthread_join(graph_thread, NULL);

	time(&timer);
	tm_info = localtime(&timer);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("\nClosing plant at %s!\n", buffer);

	pthread_exit(NULL);
}

double out_angle_function(double time)
{
	if (time <= 0)
		return 50;

	if (time < 20000)
		return 50 + time / 400;

	if (time < 30000)
		return 100;

	if (time < 50000)
		return 100 - (time - 30000) / 250;

	if (time < 70000)
		return 20 + (time - 50000) / 1000;

	if (time < 100000)
		return (40 + 20 * cos((time - 70000) * 2 * M_PI / 10000));

	return 100;
}
