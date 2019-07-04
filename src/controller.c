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

#define BEYOND_SATURATION 0
#define CONTROLLER_PERIOD \
	{                     \
		0, 100000000L      \
	}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int _requested_angle;
static volatile int _reported_angle;
static volatile int _level;

static volatile unsigned char quit;
static volatile unsigned char load = 1;

double pid(double dT, double level, double reference);

void *controller()
{
	timestamp_printf("Starting controller!\n");

	double angle = 50, reference = 60;

	struct timespec time_start, time_last, time_current;

	pthread_t graph_thread;

	int errnum;
	if ((errnum = pthread_create(&graph_thread, NULL, graphics, NULL)))
	{
		errorf("\nThread creation failed: %d\n", errnum);
	}

	now(&time_start);
	time_last = time_current = time_start;
	perspec pspec = {time_start, CONTROLLER_PERIOD};

	while (loading_graphics());
	load = 0;

	while (!quit)
	{
		pthread_mutex_lock(&mutex);
		double level = _level;
		// double reported_angle = _reported_angle;
		pthread_mutex_unlock(&mutex);

		now(&time_current);
		double T = timediff(&time_current, &time_start);
		double dT = timediff(&time_current, &time_last);
		time_last = time_current;

		//		last_angle = angle;
		angle = pid(dT, level, reference);
		
		timestamp_printf("T: %11.4f | dT: %7.4f | angle: %f", T, dT, angle);

		pthread_mutex_lock(&mutex);
		_requested_angle = angle;
		//		_reported_angle = last_angle;
		pthread_mutex_unlock(&mutex);

		update_graphics(T / 1000, level, angle, 0);

		ensure_period(&pspec);
	}

	quit_graphics();
	pthread_join(graph_thread, NULL);

	timestamp_printf("Closing controller!\n");

	pthread_exit(NULL);
}

double pid(double dT, double level, double reference)
{
	static double Max_Valve = MAX_VALUE + BEYOND_SATURATION;
	static double Min_Valve = MIN_VALUE - BEYOND_SATURATION;
	static double error_acceptable = 0.01;
	static volatile double Kp = 23;
	static volatile double Kd = 0;
	//static volatile double Kd = 2956.510641/100;
	static volatile double Ki = 0.005;
	//	static volatile double Ki = 0.05;
	static double pre_error = 0;
	static double integral = 0;
	static char saturation = 0;
	double error;
	double derivative;
	double output;

	//Beginning of the PID calculations
	error = reference - level;

	//If the error is too small then don't integrate
	if (abs(error) > error_acceptable && (double)saturation*error >= 0)
		integral += error * dT;

	derivative = (error - pre_error) / dT;

	output = Kp * error + Ki * integral + Kd * derivative;

	//Update error
	pre_error = error;

	//Saturation of the output
	return saturate(output, Min_Valve, Max_Valve, &saturation);
}

void update_controller(contpar *cpar)
{
	pthread_mutex_lock(&mutex);
	_level = cpar->level;
	_reported_angle = cpar->reported_angle;
	cpar->requested_angle = _requested_angle;
	pthread_mutex_unlock(&mutex);
}

void quit_controller()
{
	pthread_mutex_lock(&mutex);
	quit = 1;
	pthread_mutex_unlock(&mutex);
}

unsigned char loading_controller()
{
	return load;
}
