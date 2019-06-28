#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "time_utils.h"

/* Operations on timespecs. -- TAKEN FROM https://github.com/openbsd/src/blob/master/sys/sys/time.h */
#define timespecclear(tsp) (tsp)->tv_sec = (tsp)->tv_nsec = 0
#define timespecisset(tsp) ((tsp)->tv_sec || (tsp)->tv_nsec)
#define timespecisvalid(tsp) \
    ((tsp)->tv_nsec >= 0 && (tsp)->tv_nsec < 1000000000L)
#define timespeccmp(tsp, usp, cmp)                        \
    (((tsp)->tv_sec == (usp)->tv_sec)                     \
         ? ((tsp)->tv_nsec cmp (usp)->tv_nsec)            \
         : ((tsp)->tv_sec cmp (usp)->tv_sec))
#define timespecadd(tsp, usp, vsp)                        \
    do                                                    \
    {                                                     \
        (vsp)->tv_sec = (tsp)->tv_sec + (usp)->tv_sec;    \
        (vsp)->tv_nsec = (tsp)->tv_nsec + (usp)->tv_nsec; \
        if ((vsp)->tv_nsec >= 1000000000L)                \
        {                                                 \
            (vsp)->tv_sec++;                              \
            (vsp)->tv_nsec -= 1000000000L;                \
        }                                                 \
    } while (0)
#define timespecsub(tsp, usp, vsp)                        \
    do                                                    \
    {                                                     \
        (vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;    \
        (vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec; \
        if ((vsp)->tv_nsec < 0)                           \
        {                                                 \
            (vsp)->tv_sec--;                              \
            (vsp)->tv_nsec += 1000000000L;                \
        }                                                 \
    } while (0)

#define TIMESTAMP_SIZE 26

size_t get_timestamp(char *timestamp);

int timestamp_printf(const char *format, ...)
{
    char timestamp[TIMESTAMP_SIZE];
    get_timestamp(timestamp);

    va_list argptr;
    char *message;

    va_start(argptr, format);
    vasprintf(&message, format, argptr);

    int ret = printf("[%s] %s", timestamp, message);

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

int ensure_period(perspec *pspec)
{
    struct timespec time_now, sleep_time;
    timespecadd(&pspec->time_next, &pspec->period, &pspec->time_next);

    int error;
    if ((error = now(&time_now)))
    {
        return error;
    }
    
    timespecsub(&pspec->time_next, &time_now, &sleep_time);

    while (nanosleep(&sleep_time, &sleep_time) && timespeccmp(&pspec->time_next, &time_now, >))
    {
        if ((error = now(&time_now)))
        {
            return error;
        }
    }

    return 0;
}

double timediff(const struct timespec *x, const struct timespec *y)
{
    return (x->tv_sec - y->tv_sec) * 1000. + (x->tv_nsec - y->tv_nsec) / 1000000.;
}