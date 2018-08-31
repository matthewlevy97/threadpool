#include "threadpool.h"

static struct thread_pool pool;

static unsigned int threadID;

static volatile unsigned char run_maintenance;

static int drop_thread();
static void cleanup_dead_threads();
static void * maintenance(void* ptr);

int pool_init(unsigned int threads) {
#ifdef DEBUG
	printf("[*] Thread pool init\n");
#endif
	int i;
	i = 0;
	
	// Initialize static variables
	threadID = 1;
	run_maintenance = 1;
	
	// Initialize pool
	pool.pool_size    = 0;
	pool.pool_threads = 0;
	pool.threads      = NULL;
	
	// Initialize mutex
	if(pthread_mutex_init(&(pool.lock), NULL)) {
#ifdef DEBUG
		perror("[pool_init] pthread_mutex_init() failed\n");
#endif
		goto pthread_mutex_failed;
	}
	
	// Init job queue
	if(queue_init()) {
#ifdef DEBUG
		printf("[pool_init] queue_init() failed\n");
#endif
		goto queue_init_failed;
	}
	
	// Setup initial threads
	while(threads--) {
		if(pool_create_thread()) {
#ifdef DEBUG
			perror("[pool_init] pool_create_thread() failed\n");
#endif
			goto thread_creation_failed;
		}
		
		// Signal thread as initial thread
		pool.threads[pool.pool_threads - 1]->initial_thread = 1;
	}
	
	// Setup maintenance thread
	if(pthread_create(&pool.maintenance_thread, NULL, &maintenance, NULL)) {
		// Failed to create thread
#ifdef DEBUG
		perror("[pool_init] pthread_create() failed\n");
#endif
		goto maintenance_create_failed;
	}

	return 0;
maintenance_create_failed:
thread_creation_failed:
	// Clear out threads
	while(i < pool.pool_threads)
		if(pool.threads[i]) pool.threads[i]->status = THREAD_STATUS_STOPPED;
	
	// Clean up
	maintenance(NULL);
	free(pool.threads);
	
	// Destroy mutex
	pthread_mutex_destroy(&(pool.lock));
queue_init_failed:
pthread_mutex_failed:
	return -1;
}

void pool_destroy() {
#ifdef DEBUG
	printf("[*] Thread pool destroy\n");
#endif
	int i;
	i = 0;
	
	// Kill maintenance thread
	run_maintenance = 0;
	pthread_join(pool.maintenance_thread, NULL);
	
	// Clear out threads
	while(i < pool.pool_threads)
		if(pool.threads[i]) pool.threads[i]->status = THREAD_STATUS_STOPPED;

	// Clean up
	maintenance(NULL);
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
	
	// Acquire lock
	pthread_mutex_lock(&(pool.lock));
	
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
	thread                 = pool.threads[pool.pool_threads];
	thread->threadID       = threadID;
	thread->status         = THREAD_STATUS_RUNNING;
	thread->initial_thread = 0;
	
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
	
	// Release lock
	pthread_mutex_unlock(&(pool.lock));

	return 0;
pthread_create_failed:
malloc_failed:
realloc_failed:
	// Release lock
	pthread_mutex_unlock(&(pool.lock));
	
	return -1;
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
static int drop_thread() {
	int i;
	struct thread_info * thread;
	
	// Acquire lock
	pthread_mutex_lock(&(pool.lock));
	
	// No threads to drop
	if(!(pool.threads[0] && pool.pool_threads)) {
		goto fail;
	}
	
	// Get last thread
	i = 1;
	do {
		thread = pool.threads[pool.pool_threads - (i++)];
		if(thread && thread->status == THREAD_STATUS_RUNNING &&
			!(thread->initial_thread))
			break;
	} while(pool.pool_threads > i);
	
	// Did we succeed?
	if(!thread || thread->status != THREAD_STATUS_RUNNING || thread->initial_thread) {
		goto fail;
	}
	
	// Set thread status to stop
	thread->status = THREAD_STATUS_STOPPED;
	
	// Unlock lock
	pthread_mutex_unlock(&(pool.lock));
	return 1;
fail:
	// Unlock lock
	pthread_mutex_unlock(&(pool.lock));
	return 0;
}

/**
	Remove all exited threads and frees resources
	*** Not thread safe w/o mutex ***
*/
static void cleanup_dead_threads() {
	unsigned int i;
		
	for(i = 0; i < pool.pool_size; i++) {
		if(pool.threads[i] && pool.threads[i]->status == THREAD_STATUS_EXITED) {
			pthread_join(pool.threads[i]->thread, NULL);
			free(pool.threads[i]);
			pool.threads[i] = NULL;
			pool.pool_threads--;
		}
	}	
}

/**
	Run every X seconds and remove exited threads from the list
*/
static void * maintenance(void* ptr) {
	int i, j;
	while(run_maintenance) {
		// Acquire lock
		pthread_mutex_lock(&(pool.lock));
		
		cleanup_dead_threads();
		
		// Start compressing running threads
		for(i = 0, j = 0; i < pool.pool_size; i++) {
			// If we find an empty slot
			if(!(pool.threads[i])) {
				// Find the next populated slot
				while(++j < pool.pool_size && !(pool.threads[j]));
				
				// Is there another slot?
				if(j >= pool.pool_size)
					break;
				
				// Move populated slot to current
				pool.threads[i] = pool.threads[j];
				
				// NULL out old slot
				pool.threads[j] = NULL;
			}
		}
		
		// Unlock lock
		pthread_mutex_unlock(&(pool.lock));
		
		// Yield for a few seconds
		sleep(MAINTENANCE_SLEEP_DELAY);
	}
}
