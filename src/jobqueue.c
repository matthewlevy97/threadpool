#include "jobqueue.h"

struct job_queue queue;

int queue_init() {
#ifdef DEBUG
	printf("[*] Queue Init\n");
#endif
	
	queue.num_jobs = 0;
	queue.head     = NULL;
	queue.tail     = NULL;
	
	if(pthread_mutex_init(&(queue.lock), NULL)) {
#ifdef DEBUG
		perror("[queue_init] pthread_mutex_init() failed\n");
#endif
		return -1;
	}
	
	return 0;
}
void queue_destroy() {
#ifdef DEBUG
	printf("[*] Destroying Queue");
#endif
	// Clear the queue
	queue_clear();
	
	// Delete the lock
#ifdef DEBUG
	if(pthread_mutex_destroy(&(queue.lock))) {
		perror("[queue_destroy] pthread_mutex_destroy() failed\n");
	}
#else
	pthread_mutex_destroy(&(queue.lock));
#endif
}

void queue_insert(struct job_info * job) {
	struct job_entry * entry;
	
	// No job, stop now
	if(!job) return;
	
	// Create job_entry struct
	entry = malloc(sizeof(struct job_entry));
	entry->next = NULL;
	memcpy(&(entry->job), job, sizeof(struct job_info));
	
	// Lock
	pthread_mutex_lock(&(queue.lock));
	
	// Insert job_entry at tail (end) of queue
	if(queue.tail) {
		(queue.tail)->next = entry;
	} else {
		// This is the first entry
		queue.head = entry;
	}
	
	// Add element to the tail
	queue.tail = entry;
	
	// Increase job count
	queue.num_jobs++;
	
	// Unlock
	pthread_mutex_unlock(&(queue.lock));
}

/**
	Returns first job in queue
	Returns NULL if queue is empty
	Removes the entry from the queue
*/
struct job_info * queue_get() {
	struct job_entry * head;
	struct job_info  * job;
	
	// Lock
	pthread_mutex_lock(&(queue.lock));
	head = queue.head;
	job  = NULL;
	
	// See if anything is at the head to return
	if(head) {
		// Copy job information
		job = malloc(sizeof(struct job_info));
		memcpy(job, &(head->job), sizeof(struct job_info));
		
		// If head == tail, set both to NULL
		if(head != queue.tail) {
			queue.head = head->next;
		} else {
			queue.head = NULL;
			queue.tail = NULL;
		}
		
		// Remove element
		free(head);
		queue.num_jobs--;
	}
	
	// Unlock
	pthread_mutex_unlock(&(queue.lock));
	
	return job;
}

int queue_size() {
	return queue.num_jobs;
}

/**
	Remove and free every element in the queue
*/
void queue_clear() {
#ifdef DEBUG
	printf("[*] Queue Clear\n");
#endif
	
	struct job_entry * next, * prev;
	
	// Lock
	pthread_mutex_lock(&(queue.lock));
	
	next = queue.head;
	prev = NULL;
	
	// Free all job_entry structures
	while(next) {
		prev = next;
		next = (queue.head)->next;
		free(prev);
	}
	// Free tail
	free(queue.tail);
	
	// Set everything to NULL
	queue.head = NULL;
	queue.tail = NULL;
	
	// Unlock
	pthread_mutex_unlock(&(queue.lock));
}
