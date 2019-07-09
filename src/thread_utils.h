#include <pthread.h>
#include "parse.h"

typedef struct parallel_argument
{
  parscomm *pcomm;
  volatile unsigned char holder;
  volatile unsigned char granter;
  char buffer[BUFFER_SIZE];
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
} pararg;

void grant_ownership(pararg *parg, unsigned char src, unsigned char dst_mask);
void wait_for_response(pararg *parg, unsigned char src, unsigned char dst);
void release_ownership(pararg *parg, unsigned char src);
void request_ownership(pararg *parg, unsigned char id);