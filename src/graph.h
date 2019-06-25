typedef struct graph_parameters
{
    volatile double time;
    volatile double level;
    volatile double inangle;
    volatile double outangle;
	volatile int leave;
	pthread_mutex_t *mutex;
} graphpar;

void *graph(void *args);