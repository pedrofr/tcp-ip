#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <stddef.h>
#include "error.h"
#include "plant.h"
#include "graphics.h"
#include "control_utils.h"
#include "comm_consts.h"
#include "time_utils.h"

#define LEVEL_RATE 0.00002
#define VALVE_RATE 0.01
#define PLANT_PERIOD {0, 10000000L}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile double _delta;
static volatile int _max = 100;
static volatile int _level;

static volatile char load;
static volatile char quit;

static double out_angle_function(double time);

void *plant()
{
	timestamp_printf("Starting plant!\n");

	double in_angle = 50;
	double level = 0.4;

	struct timespec time_start, time_last, time_current;

	pthread_t graph_thread;

	int errnum;
	if ((errnum = pthread_create(&graph_thread, NULL, graphics, NULL)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	now(&time_start);
	time_last = time_current = time_start;
	perspec pspec = {time_start, PLANT_PERIOD};
	
	while (!quit)
	{
		pthread_mutex_lock(&mutex);
		double delta_i = _delta;
		double max = _max;
		pthread_mutex_unlock(&mutex);

		if(load)
		{
			pthread_mutex_lock(&mutex);

			timestamp_printf("Restarting plant!\n");
			restart_graphics();

			in_angle = 50;
			level = 0.4;
			delta_i = _delta = 0;
			_level = level;

			now(&time_start);
			time_last = time_current = time_start;
			pspec.time_next = time_start;

			while(loading_graphics());
			timestamp_printf("Done restarting plant!\n");

			pthread_mutex_unlock(&mutex);
			load = 0;
		}

		now(&time_current);
		double T = timediff(&time_current, &time_start);
		double dT = timediff(&time_current, &time_last);
		time_last = time_current;

		double delta_f = delta_i;

		if (delta_i > 0)
		{
			if (delta_i < VALVE_RATE * dT)
			{
				in_angle += delta_i;
				delta_f = 0;
			}
			else
			{
				in_angle += VALVE_RATE * dT;
				delta_f -= VALVE_RATE * dT;
			}
		}
		else if (delta_i < 0)
		{
			if (delta_i > -VALVE_RATE * dT)
			{
				in_angle += delta_i;
				delta_f = 0;
			}
			else
			{
				in_angle -= VALVE_RATE * dT;
				delta_f += VALVE_RATE * dT;
			}
		}

		double out_angle = out_angle_function(T);
		double influx = 1 * sin(M_PI / 2 * in_angle / 100);
		double outflux = (max / 100) * (level / 1.25 + 0.2) * sin(M_PI / 2 * out_angle / 100);
		
		level += LEVEL_RATE * dT * (influx - outflux);

		//Saturação
		level = saturate(level, 0, 1, NULL);

		// timestamp_printf("T: %11.4f | dT: %7.4f", T, dT);
		// printf(" | delta_i: %9.4f | in_angle: %9.4f | out_angle: %9.4f | level: %7.4f | influx: %f | outflux %f\n", delta_i, in_angle, out_angle, level, influx, outflux);

		pthread_mutex_lock(&mutex);
		_delta += delta_f - delta_i;
		_level = (int)round(level * 100);
		pthread_mutex_unlock(&mutex);

		update_graphics(T / 1000, level * 100, in_angle, out_angle);
		
		ensure_period(&pspec);
	}

	quit_graphics();
	pthread_join(graph_thread, NULL);

	timestamp_printf("Closing plant!\n");

	pthread_exit(NULL);
}

void update_max(int max)
{
	pthread_mutex_lock(&mutex);
	_max = max;
	pthread_mutex_unlock(&mutex);
}

void update_delta(int delta)
{
	pthread_mutex_lock(&mutex);
	_delta += delta;
	pthread_mutex_unlock(&mutex);
}

void quit_plant()
{
	quit = 1;
}

void restart_plant()
{
	load = 1;
}

char loading_plant()
{
	return load;
}

void read_level(int *level)
{
	pthread_mutex_lock(&mutex);
	*level = _level;
	pthread_mutex_unlock(&mutex);
}

static double out_angle_function(double time)
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