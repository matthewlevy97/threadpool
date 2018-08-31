#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread.h"
#include "jobqueue.h"

#ifdef DEBUG
#include <stdio.h>
#endif

// Delay in seconds
#define MAINTENANCE_SLEEP_DELAY 1

struct thread_pool {
	unsigned int          pool_size;
	unsigned int          pool_threads;
	pthread_t             maintenance_thread;
	pthread_mutex_t       lock;
	struct thread_info ** threads;
	
};

int pool_init(unsigned int threads);
void pool_destroy();

int pool_create_thread();
int pool_thread_count();

void pool_add_job(struct job_info job);

#endif
