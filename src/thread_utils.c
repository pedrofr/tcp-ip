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

void wait_for_response(pararg *parg, char id)
{
	grant_ownership(parg, id);
	request_ownership(parg, id);
}

// void wait_for_request(pararg *parg, char id)
// {
// 	pthread_mutex_lock(parg->mutex);
// 	while (parg->holder != id)
// 		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
// }

void request_ownership(pararg *parg, char id)
{
	pthread_mutex_lock(parg->mutex);
	while (parg->holder & ~id)
		pthread_cond_wait(parg->cond, parg->mutex); //wait for the cond
}

void grant_ownership(pararg *parg, char id)
{
	parg->holder = id;
	pthread_mutex_unlock(parg->mutex);
	pthread_cond_signal(parg->cond);
}

void release_ownership(pararg *parg)
{
	grant_ownership(parg, FREE);
}
