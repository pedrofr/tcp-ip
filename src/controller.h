typedef struct controller_parameters
{
	volatile int requested_angle;
	volatile int reported_angle;
	volatile int level;
	volatile int leave;
	pthread_mutex_t *mutex;
} contpar;

void *controller(void *args);