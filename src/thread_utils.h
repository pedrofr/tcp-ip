#include <pthread.h>
#include "parse.h"

typedef struct parallel_argument
{
  parscomm *pcomm;
  volatile unsigned char holder;
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
} pararg;

void wait_response(pararg *parg, char id);
void wait_request(pararg *parg, char id);
void release(pararg *parg, char id);

void wait_for_response(pararg *parg, unsigned char dst, unsigned char src);
void request_ownership(pararg *parg, unsigned char id);
void grant_ownership(pararg *parg, unsigned char mask);
void release_ownership(pararg *parg);
