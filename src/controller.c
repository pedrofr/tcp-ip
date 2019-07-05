#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <stddef.h>

#include "error.h"
#include "plant.h"
#include "control_utils.h"
#include "controller.h"
#include "comm_consts.h"
#include "time_utils.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int _angle;
static volatile int _level;

static volatile sig_atomic_t quit;
static volatile sig_atomic_t load = 1;

int pid(double dT, int level, int reference);

void *controller()
{
	timestamp_printf("Starting controller!\n");

	double reference = REFERENCE;
	struct timespec time_start, time_last, time_current;

	now(&time_start);
	time_last = time_current = time_start;
	perspec pspec = {time_start, CONTROLLER_PERIOD};
	load = 0;

	while (!quit)
	{
		pthread_mutex_lock(&mutex);
		int level = _level;
		pthread_mutex_unlock(&mutex);

		now(&time_current);
		//double T = timediff(&time_current, &time_start);
		double dT = timediff(&time_current, &time_last);
		time_last = time_current;

		//timestamp_printf("T: %11.4f | dT: %7.4f", T, dT);

		int angle = pid(dT, level, reference);

		//printf(" | angle: %f\n", angle);

		pthread_mutex_lock(&mutex);
		_angle = angle;
		pthread_mutex_unlock(&mutex);

		ensure_period(&pspec);
	}

	timestamp_printf("Closing controller!\n");

	pthread_exit(NULL);
}

int pid(double dT, int level, int reference)
{
	static double pre_error = 0;
	static double integral = 0;
	static char saturation = 0;

	static double internal_angle = INITIAL_ANGLE;

	double error;
	double derivative;
	double output;

	//Beginning of the PID calculations
	error = reference - level;

	//If the error is too small then don't integrate
	if (abs(error) > ACCEPTABLE_ERROR && (double)saturation * error >= 0)
		integral += error * dT;

	derivative = (error - pre_error) / dT;

	output = KP * error + KI * integral + KD * derivative;

	//Update error
	pre_error = error;

	//Saturation of the output
	double saturated_out = saturate(output, MIN_VALUE, MAX_VALUE, &saturation);
	double delta = saturated_out - internal_angle;

	if (delta > 0)
	{
		if (delta < VALVE_RATE * dT)
		{
			internal_angle += delta;
		}
		else
		{
			internal_angle += VALVE_RATE * dT;
		}
	}
	else if (delta < 0)
	{
		if (delta > -VALVE_RATE * dT)
		{
			internal_angle += delta;
		}
		else
		{
			internal_angle -= VALVE_RATE * dT;
		}
	}

	//printf(" | saturated_out: %f | output: %f | internal_angle: %f | delta: %f", saturated_out, output, internal_angle, delta);

	return (int)round(internal_angle);
}

void update_controller(contpar *cpar)
{
	pthread_mutex_lock(&mutex);
	_level = cpar->level;
	cpar->angle = _angle;
	pthread_mutex_unlock(&mutex);
}

void quit_controller()
{
	quit = 1;
}

sig_atomic_t loading_controller()
{
	return load;
}
