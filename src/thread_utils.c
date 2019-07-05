#include "thread_utils.h"

static volatile char request;

void wait_response(pararg *parg, char id)
{
	release(parg, id);
	wait_request(parg, id);
}

void wait_request(pararg *parg, char id)
{
	pthread_mutex_lock(parg->mutex);
	while (parg->holder == id)
		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
}

void release(pararg *parg, char id)
{
	parg->holder = id;
	pthread_mutex_unlock(parg->mutex);
	pthread_cond_signal(parg->cond);
}

void wait_for_response(pararg *parg, unsigned char src, unsigned char dst)
{
	grant_ownership(parg, src, dst);
	request_ownership(parg, src);
}

// void wait_for_request(pararg *parg, char id)
// {
// 	pthread_mutex_lock(parg->mutex);
// 	while (parg->holder != id)
// 		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
// }

void request_ownership(pararg *parg, unsigned char id)
{
	pthread_mutex_lock(parg->mutex);
	while (~parg->holder & id)
		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
	parg->holder = id;
}

void grant_ownership(pararg *parg, unsigned char src, unsigned char dst_mask)
{
	if (dst_mask)
	{
		parg->holder = dst_mask;
		parg->granter = src;
		pthread_mutex_unlock(parg->mutex);
		pthread_cond_broadcast(parg->cond);
	}
}

void release_ownership(pararg *parg, unsigned char src)
{
	grant_ownership(parg, ANY, src);
}
