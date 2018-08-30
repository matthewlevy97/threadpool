#include "threadpool.h"

static struct thread_pool pool;
static unsigned int threadID;

static void drop_thread();

int pool_init(unsigned int threads) {
	// Initialize threadID
	threadID = 0;
	
	// Initialize pool
	pool.pool_size    = 0;
	pool.pool_threads = 0;
	pool.threads      = NULL;
	
	// Init job queue
	queue_init();
	
	// Setup mutex
	if(pthread_mutex_init(&(pool.lock), NULL)) {
#ifdef DEBUG
		perror("[pool_init] pthread_mutex_init failed\n");
#endif
		goto mutex_init_failed;
	}
	
	// Setup initial threads
	for(;threads--;)
		if(pool_create_thread()) {
#ifdef DEBUG
			perror("[pool_init] pool_create_thread() failed\n");
#endif
			goto thread_creation_failed;
		}
	
	goto success;
thread_creation_failed:
	while(pool.threads) drop_thread();
mutex_init_failed:
	queue_destroy();
	return -1;
success:
	return 0;
}
void pool_destroy() {
	// Destroy threads
	while(pool.threads) drop_thread();
	free(pool.threads);
	
	// Destroy job_queue
	queue_destroy();
	
	// Destroy thread lock
#ifdef DEBUG
	if(pthread_mutex_destroy(&(pool.lock))) {
		perror("[pool_destroy] pthread_mutex_destroy() failed\n");
	}
#else
	pthread_mutex_destroy(&(pool.lock));
#endif
	
}

/**
Creates a new thread and adds it to the thread pool
*/
int pool_create_thread() {
	struct thread_info * thread;
	struct thread_info * ptr;
	
	// Resize pool
	if(pool.pool_threads >= pool.pool_size) {
		ptr = realloc(pool.threads, (pool.pool_threads + 5) * sizeof(struct thread_info));
		if(!ptr) {
			// realloc failed
#ifdef DEBUG
			perror("[pool_create_thread] realloc failed\n");
#endif	
			goto realloc_failed;
		}
		pool.pool_size = pool.pool_threads + 5;
		pool.threads = ptr;
	}
	
	// Create new thread_info
	thread = &pool.threads[pool.pool_threads];
	thread->threadID = threadID;
	thread->status   = THREAD_STATUS_STOPPED;
	if(pthread_create(&(thread->thread), NULL, &thread_init, thread)) {
		// Failed to create thread
#ifdef DEBUG
		perror("[pool_create_thread] pthread_create() failed\n");
#endif
		goto pthread_create_failed;
	}
	
	// Detach thread
#ifdef DEBUG
	if(pthread_detach(thread->thread))
		perror("[pool_create_thread] pthread_detach() failed\n");
#else
	pthread_detach(thread->thread);
#endif
	
	// Increase counters
	threadID++;
	pool.pool_threads++;
	
	goto success;
pthread_create_failed:
realloc_failed:
	return -1;
success:
	return 0;
}

int pool_thread_count() {
	return pool.pool_threads;
}

/**
	Adds a job to the queue
*/
void pool_add_job(struct job_info job) {
	queue_insert(&job);
}

/**
	Drops the last thread in the list from the pool
*/
static void drop_thread() {
	// No threads to drop
	if(!(pool.threads && pool.pool_threads)) {
		pool.threads = NULL;
		return;
	}
	
	// Set thread status to stop
	(pool.threads[pool.pool_threads]).status = THREAD_STATUS_STOPPED;
	
	// Descrease pool_threads counter
	pool.pool_threads--;
	if(!pool.pool_threads)
		pool.threads = NULL;
}

int main(int argc, char ** argv) {
	pool_init(100);
	sleep(1);
	pool_destroy();
	sleep(1);
}
