#include <signal.h>
#include "thread_utils.h"

void *control(void *args);
sig_atomic_t loading_control();
void quit_control();
