typedef struct controller_parameters
{
	volatile int requested_angle;
	volatile int reported_angle;
	volatile int level;
} contpar;

void *controller();
void update_controller(contpar *cpar);
void quit_controller();
char loading_controller();