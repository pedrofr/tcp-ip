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
