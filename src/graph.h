typedef struct graph_parameters
{
  volatile double time;
  volatile double var1; //level
  volatile double var2; //inangle
  volatile double var3; //outangle
  volatile int leave;
  pthread_mutex_t *mutex;
} graphpar;

void *graph(void *args);
