#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "time_utils.h"

#define TIMESTAMP_SIZE 26

int timestamp_printf(const char* format, ...)
{
    char timestamp[TIMESTAMP_SIZE];
    get_timestamp(timestamp);

    va_list argptr;
    char *message;

    va_start(argptr, format);
    vasprintf(&message, format, argptr);
   
	int ret = printf("[%s] %s\n", timestamp, message);

    free(message);
    va_end(argptr);

    return ret;
}

size_t get_timestamp(char *timestamp)
{
    time_t time_current = time(NULL);
	return strftime(timestamp, TIMESTAMP_SIZE, "%Y-%m-%d %H:%M:%S", localtime(&time_current));
}

int now(struct timespec *time)
{
    return clock_gettime(CLOCK_MONOTONIC_RAW, time);
}

int ensure_period(struct timespec time_initial, struct timespec period)
{
    struct timespec time_now, sleep_time;

    int error;

    if ((error = now(&time_now))) {
        return error;
    }

    sleep_time.tv_sec = period.tv_sec - (time_now.tv_sec - time_initial.tv_sec);
    sleep_time.tv_sec = sleep_time.tv_sec < 0 ? 0 : sleep_time.tv_sec;

    sleep_time.tv_nsec = period.tv_nsec - (time_now.tv_nsec - time_initial.tv_nsec);
    sleep_time.tv_nsec = sleep_time.tv_nsec < 0 ? 0 : sleep_time.tv_nsec;

    // printf("\nsleep: %lis %lins", sleep_time.tv_sec, sleep_time.tv_nsec);
    
    while (nanosleep(&sleep_time, &sleep_time) && timediff(sleep_time, period) < 0);
    
    return 0;
}

double timediff(struct timespec time1, struct timespec time0)
{
    return (time1.tv_sec - time0.tv_sec) * 1000. + (time1.tv_nsec - time0.tv_nsec) / 1000000.;
}