typedef struct plant_parameters
{
	volatile double delta;
	volatile double max;
	volatile int leave;
	volatile int level;
	pthread_mutex_t *mutex;
} plantpar;

void *plant(void *args);
double out_angle(double time);
