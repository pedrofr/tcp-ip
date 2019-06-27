#include <time.h>

#define TIMESTAMP_SIZE 26

size_t get_timestamp(char *timestamp);
int now(struct timespec *time);
int ensure_period(struct timespec time_initial, struct timespec period);
double timediff(struct timespec time1, struct timespec time0);
int timestamp_printf(const char* format, ...);