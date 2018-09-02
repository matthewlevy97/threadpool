# Threadpool
Simple threadpool library implemented in C.

## How it works
When the threadpool is created, you specify the minimum number of threads to have running in the pool at any time.
From there you can add jobs to the job queue.  The job queue takes as a parameter a structure that contains the same type of information normally passed to a pthread.
This includes: a function (void*) and an argument (void*).  No return values are returned from calling the inserted functions.

### Adding threads
Any thread added after the pool was created are designated disposable. This means that once the queue is empty, they will start dying off until only the minimum number of threads remain.

### Adding jobs
Jobs are structures that contain a pointer to a void* function and contain a void* argument.  It is a good assumption to treat this the same way you would if you executed *pthread_create(NULL, &function, NULL, argument)*

## Example code
See simple_test.c in the src/ folder.
