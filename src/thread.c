#include "thread.h"

static void execute(struct thread_info * self);

void * thread_init(void * arg) {
	execute(arg);
	
	// If we need to say that we are exiting
	((struct thread_info *)arg)->status = THREAD_STATUS_EXITED;
	
	pthread_exit(NULL);
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
			} else if(!(self->initial_thread)) {
				self->status = THREAD_STATUS_STOPPED;
			}
			break;
		}
	}
}
