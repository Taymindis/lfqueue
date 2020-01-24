

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "lfqueue.h"

#ifndef LFQ_WINDOWS
#error This is Windows only code
#endif

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h>    /* _beginthread, _endthread */

/*----------------------------------------------------------------------------------------------*/
/*
 * Structure used in select() call, taken from the BSD file sys/time.h.
 */
struct timeval {
	long    tv_sec;         /* seconds */
	long    tv_usec;        /* and microseconds */
};

struct timeval  tv1, tv2;

lfqueue_t the_queue;

#define MAX_THREADS 8
#define total_put 50000
/*----------------------------------------------------------------------------------------------*/
unsigned __stdcall worker(void *);
/*----------------------------------------------------------------------------------------------*/
unsigned __stdcall worker(void *arg)
{
	int i = 0;
	int *int_data;
	while (i < total_put) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i++;
		/*Enqueue*/

		while (lfqueue_enq(&the_queue, int_data)) {
			printf("ENQ FULL?\n");
		}
		
		/*Dequeue*/
		while ((int_data = lfqueue_deq(&the_queue)) == NULL) {
			// usleep(1000);
			printf("DEQ EMPTY?\n");
		}
		free(int_data);
	}
	return 0;
}

/*----------------------------------------------------------------------------------------------*/
#define JOIN_ALL_THREADS \
for (i = 0; i < MAX_THREADS; i++)\
WaitForSingleObject(threads[i], INFINITE)

/*----------------------------------------------------------------------------------------------*/
/*
#define detach_thread_and_loop \
for (i = 0; i < MAX_THREADS; i++)\
pthread_detach(threads[i]);\
while (1) {\
sleep(2);\
printf("current size= %zu\n", lfqueue_size(&the_queue) );\
}*/

/*----------------------------------------------------------------------------------------------*/
int main(int argc, char ** argv)
{
	//const static int MAX_THREADS = 2;//sysconf(_SC_NPROCESSORS_ONLN); // Linux

	static const int LOOP_SIZE_ = 100;
	int i, n; 

	if (lfqueue_init(&the_queue) == -1) {
		perror("LF QUEUE initialization has failed.");
		return EXIT_FAILURE ;
	}

	printf("\n\n");
	clock_t total_start = clock();
	
	for (n = 0; n < LOOP_SIZE_; n++) {

		/* Spawn the threads. */
		HANDLE threads[MAX_THREADS] = {0};

		clock_t start = clock();
		/* inside each loop begin workers on separate threads */
		for (i = 0; i < MAX_THREADS; i++) 
		{
			unsigned udpthreadid = 0;
			threads[i] = (HANDLE)_beginthreadex(NULL, 0, worker, NULL, 0, &udpthreadid);
		}

		JOIN_ALL_THREADS;
		// detach_thread_and_loop;

		printf("\rFinished loop [%3d] of [%3d], Total time to begin and join [%3d] threads was:  [%3.3f] seconds ", 
			n, LOOP_SIZE_, MAX_THREADS, ((float)(clock() - start) / CLOCKS_PER_SEC));

		if (0 != lfqueue_size(&the_queue))
		{
			perror("Error, all queues should be consumed but they are not");
				return EXIT_FAILURE;
		}

	} /* main loop */

	lfqueue_destroy(&the_queue);

	printf("\n\n");
	printf("Total test time: %3.3f minutes", ((float)(clock() - total_start) / CLOCKS_PER_SEC) / 60.0 );
	printf("\n\n");

	system("@echo.");
	system("@echo --- Test has passed --- ");
	system("@echo.");
	system("@pause.");

	return EXIT_SUCCESS;
}

