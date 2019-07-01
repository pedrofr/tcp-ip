#include <pthread.h>
#include "parse.h"

typedef struct parallel_argument
{
  parscomm *pcomm;
  volatile int holder;
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
} pararg;

void wait_response(pararg *parg, int id);
void wait_request(pararg *parg, int id);
void release(pararg *parg, int id);
