#ifndef __JOBQUEUE_H
#define __JOBQUEUE_H

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#ifdef DEBUG
#include <stdio.h>
#endif

struct job_info {
	void (*func)(void*);
	void * arg;
};

struct job_entry {
	struct job_info     job;
	struct job_entry *  next;
};

struct job_queue {
	unsigned int        num_jobs;
	struct job_entry *  head; // POP
	struct job_entry *  tail; // PUSH
	pthread_mutex_t     lock;
};

int queue_init();
void queue_destroy();

void queue_insert(struct job_info *);
struct job_info * queue_get();

int queue_size();
void queue_clear();

#endif
