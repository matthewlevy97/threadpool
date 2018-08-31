#ifndef __THREAD_H
#define __THREAD_H

#include <signal.h>

#include "jobqueue.h"

enum thread_status {
	THREAD_STATUS_STOPPED,
	THREAD_STATUS_RUNNING,
	THREAD_STATUS_PAUSED,
	THREAD_STATUS_EXITED,
};

struct thread_info {
	int                 threadID;
	enum thread_status  status;
	pthread_t           thread;
};

void * thread_init(void * arg);

#endif
