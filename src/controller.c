#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <stddef.h>
#include "error.h"
#include "plant.h"
#include "graphics.h"
#include "control_utils.h"
#include "controller.h"
#include "comm_consts.h"
#include "time_utils.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int _leave;
static volatile int _requested_angle;
static volatile int _reported_angle;
static volatile int _level;

double pid(double dT, double level, double angle, double reference);
double bang_bang(double level, double angle, double reference);

void *controller()
{
	double angle;
	double reference = 80;

	struct timespec time_start, time_last, time_current;
	struct timespec sleep_time = {0, 20000000L};

  	timestamp_printf("Starting controller!");

	pthread_t graph_thread;

	int errnum;
	if ((errnum = pthread_create(&graph_thread, NULL, graphics, NULL)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	now(&time_start);
	time_last = time_current = time_start;
	time_last = time_current = time_start;

	while (1)
	{
		pthread_mutex_lock(&mutex);
		int leave = _leave;
		double level = _level;
		double reported_angle = _reported_angle;
		pthread_mutex_unlock(&mutex);

		now(&time_current);
		double T = timediff(time_current, time_start);
		double dT = timediff(time_current, time_last);
		time_last = time_current;

		angle = pid(dT, level, reported_angle, reference);
		angle = bang_bang(level, reported_angle, reference);

		// timestamp_printf("T: %11.4f | dT: %7.4f", T, dT);
		// printf(" | delta_i: %9.4f | in_angle: %9.4f | out_angle: %9.4f | level: %7.4f | influx: %f | outflux %f", delta_i, in_angle, out_angle, level, influx, outflux);

		pthread_mutex_lock(&mutex);
		_requested_angle = (int)round(angle);
		pthread_mutex_unlock(&mutex);

		update_graphics(T / 1000, level, reported_angle, 0);

		if (leave)
			break;

		ensure_period(time_current, sleep_time);
	}

	quit_graphics();
	pthread_join(graph_thread, NULL);

  	timestamp_printf("Starting controller!");

	pthread_exit(NULL);
}

double pid(double dT, double level, double angle, double reference)
{
	//TODO: Calculos do controlador
	return dT * reference * angle * level;
} 

double bang_bang(double level, double angle, double reference)
{
	if (level < reference)
	{
		return 100;
	}
	else if (level > reference)
	{
		return 0;
	}
	else
	{
		return angle;
	}
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