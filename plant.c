#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include "error.h"
#include "plant.h"

void *plant(void *args)
{
	volatile plantpar *ppar = (plantpar *)args;
	double in_angle = 50;
	double level = 0.4;
	struct timespec time_initial, time_last, time_current;

	clock_gettime(CLOCK_MONOTONIC_RAW, &time_initial);
	time_current = time_initial;
	time_last = time_current;

	char buffer[26];
    time_t timer;
    time(&timer);
    struct tm* tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("\nStarting plant at %s!\n", buffer);

	while (1)
	{
		pthread_mutex_lock(ppar->mutex);
		int leave = ppar->leave;
		double delta = ppar->delta;
		double max = ppar->max;
		pthread_mutex_unlock(ppar->mutex);

		if (leave)
			break;

		clock_gettime(CLOCK_MONOTONIC_RAW, &time_current);
		double T = (time_current.tv_sec - time_initial.tv_sec) * 1000. + (time_current.tv_nsec - time_initial.tv_nsec) / 1000000.;
		double dT = (time_current.tv_sec - time_last.tv_sec) * 1000. + (time_current.tv_nsec - time_last.tv_nsec) / 1000000.;
		time_last = time_current;

		if (delta > 0)
		{
			if (delta < 0.01 * dT)
			{
				in_angle += delta;
				delta = 0;
			}
			else
			{
				in_angle += 0.01 * dT;
				delta -= 0.01 * dT;
			}
		}
		else if (delta < 0)
		{
			if (delta > -0.01 * dT)
			{
				in_angle += delta;
				delta = 0;
			}
			else
			{
				in_angle -= 0.01 * dT;
				delta += 0.01 * dT;
			}
		}

		double influx = 1 * sin(M_PI / 2 * in_angle / 100);
		double outflux = (max / 100) * (level / 1.25 + 0.2) * sin(M_PI / 2 * out_angle(T) / 100);
		level += 0.00002 * dT * (influx - outflux);
		// level += 0.002 * dT * (influx - outflux);

		printf("\nT: %11.4f | dT: %7.4f", T, dT);

		printf(" | delta: %9.4f | in_angle: %9.4f | level: %7.4f | influx: %f | outflux %f", delta, in_angle, level, influx, outflux);

		pthread_mutex_lock(ppar->mutex);
		ppar->delta = delta;
		ppar->level = level;
		pthread_mutex_unlock(ppar->mutex);

		for (int i = 0; i < 100*__INT16_MAX__; i++) { }
	}

    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("\nClosing plant at %s!\n", buffer);

	pthread_exit(NULL);
}

double out_angle(double time)
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