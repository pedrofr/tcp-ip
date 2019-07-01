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

#define CONTROLLER_PERIOD {0, 20000000L}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int _quit;
static volatile int _requested_angle;
static volatile int _reported_angle;
static volatile int _level;

double pid(double dT, double level, double reference);

void *controller()
{
  	timestamp_printf("Starting controller!\n");

	double angle;
	double reference = 80;
	int quit = 0;

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

	while (!quit)
	{
		pthread_mutex_lock(&mutex);
		quit = _quit;
		double level = _level;
		double reported_angle = _reported_angle;
		pthread_mutex_unlock(&mutex);

		now(&time_current);
		double T = timediff(&time_current, &time_start);
		double dT = timediff(&time_current, &time_last);
		time_last = time_current;

		angle = pid(dT, level, reference);

		timestamp_printf("T: %11.4f | dT: %7.4f", T, dT);

		pthread_mutex_lock(&mutex);
		_requested_angle = (int)round(angle);
		pthread_mutex_unlock(&mutex);

		update_graphics(T / 1000, level, reported_angle, 0);

		ensure_period(&pspec);
	}

	quit_graphics();
	pthread_join(graph_thread, NULL);

  	timestamp_printf("Closing controller!\n");

	pthread_exit(NULL);
}


double pid(double dT, double level, double reference)
{
	static double Max_Valve = 100;
	static double Min_Valve =0;
	static double error_acceptable = 0.01;
	static volatile double Kp =22.94;
	static volatile double Kd =2956.510641;
	static volatile double Ki =0.00005;
	static double pre_error =0;
	static double integral =0;
	double error;
	double derivative;
	double output;
	//Beginning of the PID calculations
	error = reference - level;
	//If the error is too small then don't integrate
	if(abs(error) >error_acceptable)
		integral = integral +error*dT;
	derivative = (error - pre_error)/dT;
    output = Kp*error + Ki*integral +Kd*derivative;
	//Update error
	pre_error = error;
	
	 //Saturation of the output
	if(output > Max_Valve)
	{
		output = Max_Valve;
	}
	if(output < Min_Valve)
	{
		output = Min_Valve;
	}
	return output;
	
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
	_quit = 1;
	pthread_mutex_unlock(&mutex);
}

void read_controller(contpar *cpar)
{
	pthread_mutex_lock(&mutex);
	cpar->requested_angle = _requested_angle;
	pthread_mutex_unlock(&mutex);
}