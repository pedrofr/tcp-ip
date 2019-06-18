#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include "error.h"
#include "parse.h"
#include "simulator.h"
#include "comm_consts.h"

void wait_request(pararg *parg, int id);
void release(pararg *parg, int id);

typedef struct Simulation_parameters
{
  double *delta;
  double *Max;
  pthread_mutex_t *mutex;
  int *leave;
}simpar;

void *plant(void *args);

void *simulate(void *args)
{
	pararg *parg = (pararg *)args;

	parscomm *pcomm = parg->pcomm;
	int *mod = parg->mod;

	char command[256];
	char argument[256];
	int errnum;
	char buffer_out[256];

	printf("\nStarting simulator!\n");

	int leave = 0;
	double delta = 0;
	double Max = 100;
	pthread_t plant_thread;

	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	simpar spar  = {&delta,&Max,&mutex,&leave};

	if ((errnum = pthread_create(&plant_thread, NULL, plant, &spar)))
	  {
	    sprintf(buffer_out, "Thread creation failed: %d\n", errnum);
	    error(buffer_out);
	  }

	while (1)
	{
		wait_request(parg, SIMULATOR);

		if (matches_numeric(pcomm->command, pcomm->argument, "OpenValve"))
		{
			double value = atof(pcomm->argument);
			pthread_mutex_lock(&mutex);
			delta += value;
			pthread_mutex_unlock(&mutex);
			sprintf(argument, "%i", (int)value);
			strcpy(pcomm->command, "Open");
			strcpy(pcomm->argument, argument);
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "CloseValve"))
		{
			double value = atof(pcomm->argument);
			pthread_mutex_lock(&mutex);
			delta -= value;
			pthread_mutex_unlock(&mutex);
			sprintf(argument, "%i", (int)value);
			strcpy(pcomm->command, "Close");
			strcpy(pcomm->argument, argument);
		}
		else if (matches_numeric(pcomm->command, pcomm->argument, "SetMax"))
		{
			double value = atof(pcomm->argument);
			pthread_mutex_lock(&mutex);
			Max = value;
			pthread_mutex_unlock(&mutex);
			sprintf(argument, "%i", (int)value);
			strcpy(pcomm->command, "Max");
			strcpy(pcomm->argument, argument);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "GetLevel"))
		{
		  
			pthread_mutex_lock(&mutex);
			sprintf(argument, "%i", (int)delta);
			pthread_mutex_unlock(&mutex);
			strcpy(pcomm->command, "Level");
			strcpy(pcomm->argument, argument);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "CommTest"))
		{
			strcpy(pcomm->command, "Comm");
			strcpy(pcomm->argument, OK);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Start"))
		{
			strcpy(pcomm->command, "Start");
			strcpy(pcomm->argument, OK);
		}
		else if (matches_no_arg(pcomm->command, pcomm->argument, "Exit"))
		{
			strcpy(pcomm->command, "Exit");
			strcpy(pcomm->argument, OK);
			pthread_mutex_lock(&mutex);
			leave = 1;
			pthread_mutex_unlock(&mutex);
		}
		else
		{
			strcpy(pcomm->command, "NoMessageFound!");
			strcpy(pcomm->argument, "");
		}

		release(parg, SIMULATOR);

		if (leave)
			break;
	}

	printf("\nClosing simulator!\n");
}

void wait_response(pararg *parg, int id)
{
	release(parg, id);
	wait_request(parg, id);
}

void wait_request(pararg *parg, int id)
{
	pthread_mutex_lock(parg->mutex);
	while (*(parg->mod) == id)
		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
}

void *plant(void *args)
{
  simpar *spar = (simpar *)args;
  double in_angle = 50;
  double influx;
  double outflux;
  double level=0.4;
  time_t time_initial,time_last,time_current;

  time(&time_initial);
  time_current = time_initial;
  time_last = time_current;
  
  while(1)
    {
      pthread_mutex_lock(spar->mutex);
      double del =spar-> delta;
      double mx =spar-> Max;
      pthread_mutex_unlock(spar->mutex);
      
      time(&time_current);
      double time_total = 1000* difftime(time_current,time_init);
      double deltat = 1000* difftime(time_current,time_last); 

      if(del>0)
	{
	  if(del<0.01*deltat)
	    {
              in_angle = in_angle + del;
	      del =0;
	    }
	  else
	    {
	      in_angle  += 0.01*deltat;
	      del -= 0.01*deltat;
	    }
	}
      else if(del<0)
	{
	  if(del>-0.01*deltat)
	    {
	      in_angle += del;
	      del =0;
	    }
	  else
	    {
	      in_angle  -= 0.01*deltat;
	      del += 0.01*deltat;
	    }
	}

      influx = 1*sin(pi()/2*in_angle/100);
      outflux = (mx/100)*(level/1.25+0.2)*sin(pi()/2*out_angle(time_total)/100);
      level += 0.00002*deltat*(influx-outflux);
      time_last = time_current;
      
      pthread_mutex_lock(spar->mutex);
     spar-> delta = del;
     spar-> Max = mx;
      pthread_mutex_unlock(spar->mutex);
    }
}


void *simulate(void *args)
double out_angle(double time)
{
  if(time<=0)
    return 50;
  if(time<20000)
    return (50 + time/400);
  if(time<30000)
    return 100;
  if(time<50000)
    return(100-(time-30000)/250);
  if(time<70000)
    return (20+time-50000)/1000);
  if(time<100000)
    return(40+20*cos((time-70000)*2*pi()/10000));

}

void release(pararg *parg, int id)
{
	*(parg->mod) = id;
	pthread_mutex_unlock(parg->mutex);
	pthread_cond_signal(parg->cond);
}
