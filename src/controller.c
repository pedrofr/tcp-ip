#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <stddef.h>
#include "error.h"
#include "plant.h"
#include "graphics.h"
#include "control_utilities.h"
#include "controller.h"
#include "comm_consts.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int _leave;
static volatile int _requested_angle;
static volatile int _reported_angle;
static volatile int _level;

void *controller()
{
	struct timespec time_initial, time_current;
	// struct timespec time_last;

	clock_gettime(CLOCK_MONOTONIC_RAW, &time_initial);
	time_current = time_initial;
	// time_last = time_current;

	char buffer[26];
	time_t timer;
	time(&timer);
	struct tm *tm_info = localtime(&timer);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("\nStarting controller at %s!\n", buffer);

	struct timespec sleepTime = {0, 20000000L};

	pthread_t graph_thread;

	int errnum;
	if ((errnum = pthread_create(&graph_thread, NULL, graphics, NULL)))
	{
		char buffer_out[BUFFER_SIZE];
		sprintf(buffer_out, "Thread creation failed: %d\n", errnum);
		error(buffer_out);
	}

	double angle;

	while (1)
	{
		pthread_mutex_lock(&mutex);
		int leave = _leave;
		double level = _level;
		double reported_angle = _reported_angle;
		pthread_mutex_unlock(&mutex);

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

		pthread_mutex_lock(&mutex);
		_requested_angle = (int)round(angle);
		pthread_mutex_unlock(&mutex);

		update_graphics(T / 1000, level, reported_angle, 0);

		if (leave)
			break;

		nanosleep(&sleepTime, NULL);
	}

	quit_graphics();
	pthread_join(graph_thread, NULL);

	time(&timer);
	tm_info = localtime(&timer);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("\nClosing controller at %s!\n", buffer);

	pthread_exit(NULL);
}

void update_controller(contpar cpar)
{
	pthread_mutex_lock(&mutex);
	_level = cpar.level;
	_reported_angle = cpar.reported_angle;
	pthread_mutex_unlock(&mutex);
}

void quit_controller()
{
	pthread_mutex_lock(&mutex);
	_leave = 1;
	pthread_mutex_unlock(&mutex);
}

void read_controller(contpar *cpar)
{
	pthread_mutex_lock(&mutex);
	cpar->requested_angle = _requested_angle;
	pthread_mutex_unlock(&mutex);
}