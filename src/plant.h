typedef struct plant_parameters
{
	volatile double delta;
	volatile double max;
	volatile double level;
	volatile int leave;
	pthread_mutex_t *mutex;
} plantpar;

void *plant(void *args);
