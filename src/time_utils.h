#include <time.h>

typedef struct periodic_timespec
{
    struct timespec time_next;
    struct timespec period;
} perspec;

int timestamp_printf(const char* format, ...);
int timestamp_force_printf(const char* format, ...);
double timediff(const struct timespec *x, const struct timespec *y);
int now(struct timespec *time);
int ensure_period(perspec *pspec);
void suspend_timed_output();
void restore_timed_output();
