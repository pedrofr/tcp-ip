#include <signal.h>

typedef struct controller_parameters
{
	volatile int angle;
	volatile int level;
} contpar;

void *controller();
void update_controller(contpar *cpar);
void quit_controller();
sig_atomic_t loading_controller();