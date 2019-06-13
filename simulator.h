
typedef struct parallel_argument
{
  parscomm* pcomm;
  int* mod;
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
} pararg;

void *simulate(void *args);
void wait_response(pararg *parg, int id);