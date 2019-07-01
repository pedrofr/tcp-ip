#include "thread_utils.h"

void wait_response(pararg *parg, int id)
{
	release(parg, id);
	wait_request(parg, id);
}

void wait_request(pararg *parg, int id)
{
	pthread_mutex_lock(parg->mutex);
	while (parg->holder == id)
		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
}

void release(pararg *parg, int id)
{
	parg->holder = id;
	pthread_mutex_unlock(parg->mutex);
	pthread_cond_signal(parg->cond);
}
