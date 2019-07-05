#include <signal.h>

void *plant();
void update_max(int max);
void update_delta(int delta);
void quit_plant();
void read_level(int *level);
void restart_plant();
sig_atomic_t loading_plant();