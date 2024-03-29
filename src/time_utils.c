#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "time_utils.h"

/* Operations on timespecs. -- TAKEN FROM https://github.com/openbsd/src/blob/master/sys/sys/time.h */
#define timespecclear(tsp) (tsp)->tv_sec = (tsp)->tv_nsec = 0
#define timespecisset(tsp) ((tsp)->tv_sec || (tsp)->tv_nsec)
#define timespecisvalid(tsp) \
    ((tsp)->tv_nsec >= 0 && (tsp)->tv_nsec < 1000000000L)
#define timespeccmp(tsp, usp, cmp)            \
    (((tsp)->tv_sec == (usp)->tv_sec)         \
         ? ((tsp)->tv_nsec cmp(usp)->tv_nsec) \
         : ((tsp)->tv_sec cmp(usp)->tv_sec))
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

static volatile sig_atomic_t write_allowed = 1;

size_t get_timestamp(char *timestamp);

int timestamp_printf(const char *format, ...)
{
    if (write_allowed)
    {
        char timestamp[TIMESTAMP_SIZE];
        get_timestamp(timestamp);

        va_list argptr;
        char *message;

        va_start(argptr, format);
        vasprintf(&message, format, argptr);

        int ret = printf("\033[2K\r[%s] %s", timestamp, message);

        fflush(stdout);

        free(message);
        va_end(argptr);
        return ret - TIMESTAMP_SIZE;
    }
    
    return 0;
}

int timestamp_force_printf(const char *format, ...)
{
    char timestamp[TIMESTAMP_SIZE];
    get_timestamp(timestamp);

    va_list argptr;
    char *message;

    va_start(argptr, format);
    vasprintf(&message, format, argptr);

    int ret = printf("\033[2K\r[%s] %s", timestamp, message);

    fflush(stdout);

    free(message);
    va_end(argptr);
    return ret - TIMESTAMP_SIZE;
}

size_t get_timestamp(char *timestamp)
{
    time_t time_current = time(NULL);
    return strftime(timestamp, TIMESTAMP_SIZE, "%Y-%m-%d %H:%M:%S", localtime(&time_current));
}

int now(struct timespec *time)
{
    return clock_gettime(CLOCK_MONOTONIC, time);
}

int ensure_period(perspec *pspec)
{
    timespecadd(&pspec->time_next, &pspec->period, &pspec->time_next);

    int ret;
    while ((ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pspec->time_next, NULL)));

    return ret;
}

double timediff(const struct timespec *x, const struct timespec *y)
{
    return (x->tv_sec - y->tv_sec) * 1000. + (x->tv_nsec - y->tv_nsec) / 1000000.;
}

void suspend_timed_output()
{
	write_allowed = 0;
}

void restore_timed_output()
{
	write_allowed = 1;
}