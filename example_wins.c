#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "lfqueue.h"
#include <Windows.h>
#include <time.h>

struct timeval  tv1, tv2;
lfqueue_t results;

#define total_put 50000

unsigned __stdcall worker(void *);
unsigned __stdcall worker(void *arg)
{
	int i = 0;
	int *int_data;
	while (i < total_put) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i++;
		/*Enqueue*/

		while (lfqueue_enq(&results, int_data)) {
			printf("ENQ FULL?\n");
		}

		/*Dequeue*/
		while ((int_data = lfqueue_deq(&results)) == NULL) {
			// usleep(1000);
			printf("DEQ EMPTY?\n");
		}
		free(int_data);
	}
	return 0;
}

#define join_threads \
for (i = 0; i < nthreads; i++)\
WaitForSingleObject(threads[i], INFINITE);\
printf("current size= %d\n", (int) lfqueue_size(&results) )
/*
#define detach_thread_and_loop \
for (i = 0; i < nthreads; i++)\
pthread_detach(threads[i]);\
while (1) {\
sleep(2);\
printf("current size= %zu\n", lfqueue_size(&results) );\
}*/

#define nthreads 8

int main(void)
{
	//const static int nthreads = 2;//sysconf(_SC_NPROCESSORS_ONLN); // Linux
	int i;

	if (lfqueue_init(&myq, total_put, nthreads, 1) == -1)
		return -1;

	/* Spawn threads. */
	printf("Total threads = %d\n", nthreads);
	clock_t start = clock();
	HANDLE threads[nthreads];

	for (i = 0; i < nthreads; i++) {
		unsigned udpthreadid; 
		threads[i] = (HANDLE)_beginthreadex(NULL, 0, worker, NULL, 0, &udpthreadid);
	}

	join_threads;
	// detach_thread_and_loop;

	clock_t end = clock();

	printf("Total time = %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);

	assert(0 == lfqueue_size(&results) && "Error, all queue should be consumed but not");

	lfqueue_destroy(&results);
	printf("Press Any Key to Continue\n");
	getchar();
	return 0;
}

