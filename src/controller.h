typedef struct controller_parameters
{
	volatile double delta;
	volatile double max;
	volatile double level;
	volatile int leave;
	pthread_mutex_t *mutex;
} contpar;

void *controller(void *args);