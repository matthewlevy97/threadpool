#include "threadpool.h"

static struct thread_pool pool;
static unsigned int threadID;

static void drop_thread();
static void cleanup_dead_threads();

int pool_init(unsigned int threads) {
#ifdef DEBUG
	printf("[*] Thread pool init\n");
#endif
	
	// Initialize threadID
	threadID = 0;
	
	// Initialize pool
	pool.pool_size    = 0;
	pool.pool_threads = 0;
	pool.threads      = NULL;
	
	// Init job queue
	queue_init();
		
	// Setup initial threads
	while(threads--)
		if(pool_create_thread()) {
#ifdef DEBUG
			perror("[pool_init] pool_create_thread() failed\n");
#endif
			goto thread_creation_failed;
		}
	
	goto success;
thread_creation_failed:
	while(pool.pool_threads) drop_thread();
	queue_destroy();
	return -1;
success:
	return 0;
}
void pool_destroy() {
#ifdef DEBUG
	printf("[*] Thread pool destroy\n");
#endif
	
	// Destroy threads
	while(pool.pool_threads) drop_thread();
	cleanup_dead_threads();
	free(pool.threads);
	
	// Destroy job_queue
	queue_destroy();	
}

/**
Creates a new thread and adds it to the thread pool
*/
int pool_create_thread() {
	struct thread_info * thread;
	struct thread_info ** ptr;
	
	// Resize pool
	if(pool.pool_threads >= pool.pool_size) {
		ptr = realloc(pool.threads, (pool.pool_threads + 5) * sizeof(struct thread_info *));
		if(!ptr) {
			// realloc failed
#ifdef DEBUG
			perror("[pool_create_thread] realloc failed\n");
#endif	
			goto realloc_failed;
		}
		pool.threads = ptr;
		
		// Create new threads and add to pool
		do {
			thread = malloc(sizeof(struct thread_info));
			if(!thread) {
#ifdef DEBUG
				perror("[pool_create_thread] malloc failed\n");
#endif
				goto malloc_failed;
			}
			// Wipe thread data
			memset(thread, 0, sizeof(struct thread_info));
			
			// Needed to allow for cleanup of un-used thread handles
			thread->status = THREAD_STATUS_EXITED;
			
			pool.threads[pool.pool_size++] = thread;
		} while(pool.pool_size < pool.pool_threads + 5);
	}
	
	// Create new thread_info
	thread = pool.threads[pool.pool_threads];
	thread->threadID = threadID;
	thread->status   = THREAD_STATUS_RUNNING;
	if(pthread_create(&(thread->thread), NULL, &thread_init, thread)) {
		// Failed to create thread
#ifdef DEBUG
		perror("[pool_create_thread] pthread_create() failed\n");
#endif
		goto pthread_create_failed;
	}
	
	// Increase counters
	threadID++;
	pool.pool_threads++;
	
	goto success;
pthread_create_failed:
malloc_failed:
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
	struct thread_info * thread;
	
	// No threads to drop
	if(!(pool.threads[0] && pool.pool_threads)) {
		return;
	}
	
	// Get last thread
	thread = pool.threads[--pool.pool_threads];
	
	// Set thread status to stop
	thread->status = THREAD_STATUS_STOPPED;
}

/**
	Remove all exited threads and frees resources
*/
static void cleanup_dead_threads() {
	unsigned int i;
	for(i = 0; i < pool.pool_size; i++) {
		if(pool.threads[i] && pool.threads[i]->status == THREAD_STATUS_EXITED) {
			pthread_join(pool.threads[i]->thread, NULL);
			free(pool.threads[i]);
			pool.threads[i] = NULL;
		}
	}
}

/**
	Simple testing function
*/
struct tester_arg {
	int d;
};
void * tester(void * arg) {
	sleep(1);
	printf("d=%d\n", ((struct tester_arg*)arg)->d);
	fflush(stdout);
	return NULL;
}

int main(int argc, char ** argv) {
	struct job_info job;
	int i;
	
	job.func = tester;
	job.arg  = NULL;
	
	pool_init(5);
	
	// Populate job queue
	for(i = 0; i < 300; i++) {
		job.arg = malloc(sizeof(struct tester_arg));
		((struct tester_arg*)job.arg)->d = i;
		pool_add_job(job);
	}
	
	// Add 15 threads
	for(i = 0; i < 45; i++)
		pool_create_thread();
	
	while(queue_size() > 0);
	
	printf("EXITING: %d", queue_size());
	pool_destroy();
	sleep(1);
}
