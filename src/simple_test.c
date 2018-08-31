#include "threadpool.h"

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

static void add_jobs(int jobs) {
	struct job_info job;
	int i;
	
	job.func = &tester;
	job.arg  = NULL;
	// Populate job queue
	for(i = 0; i < jobs; i++) {
		job.arg = malloc(sizeof(struct tester_arg));
		((struct tester_arg*)job.arg)->d = i;
		pool_add_job(job);
	}

}

int main(int argc, char ** argv) {
	int i;
	
	// Initialize the pool with min thread count of 5
	pool_init(5);
	
	// Add 300 jobs
	add_jobs(300);
	
	// Add 45 threads
	for(i = 0; i < 45; i++)
		pool_create_thread();
	
	sleep(2);
	
	//while(queue_size() > 0);
	
	while(1) {
		printf("QUEUE_SIZE = %d\nTHREAD_COUNT = %d\n", queue_size(), pool_thread_count());
		
		if(!queue_size())
			add_jobs(15);
		
		sleep(1);
	}
	
	printf("EXITING: %d", queue_size());
	pool_destroy();
	sleep(1);
}
