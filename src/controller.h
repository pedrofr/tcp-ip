typedef struct controller_parameters
{
	volatile double requested_angle;
	volatile double reported_angle;
	volatile double level;
} contpar;

void *controller();
void update_controller(contpar *cpar);
void quit_controller();
unsigned char loading_controller();