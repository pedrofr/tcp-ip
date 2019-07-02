#include <pthread.h>
#include "parse.h"

typedef struct parallel_argument
{
  parscomm *pcomm;
  volatile char holder;
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
} pararg;

void wait_response(pararg *parg, char id);
void wait_request(pararg *parg, char id);
void release(pararg *parg, char id);

void wait_for_response(pararg *parg, char id);
void request_ownership(pararg *parg, char id);
void grant_ownership(pararg *parg, char id);
void release_ownership(pararg *parg);
