#include "thread.h"

static int test = 0;

static void execute(struct thread_info * self);

void * thread_init(void * arg) {
	execute(arg);
	
	pthread_exit((void*)0);
}

static void execute(struct thread_info * self) {
	struct job_info * job;
	while(1) {
		switch(self->status)
		{
		case THREAD_STATUS_STOPPED: return;
		case THREAD_STATUS_PAUSED: continue;
		case THREAD_STATUS_RUNNING:
			job = queue_get();
			if(job) {
				// Execute function
				(*(job->func))(job->arg);
				free(job);
			}
			break;
		}
	}
}
