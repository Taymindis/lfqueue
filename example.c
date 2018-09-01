#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
#include <stdio.h>
#include "lfqueue.h"

struct timeval  tv1, tv2;
lfqueue_t myq;

#define total_put 5
int nthreads = 4; //sysconf(_SC_NPROCESSORS_ONLN); // Linux
/** Worker Keep Consuming at the same time, do not try instensively **/
void*  worker_c(void *);
void*  worker_c(void *arg) {
	int i = 0;
	int *int_data;
	while (i++ < total_put * nthreads) {

		/*Dequeue*/
		while ((int_data = lfqueue_deq(&myq)) == NULL) {
			usleep(1);
			// __sync_synchronize();
		}
		//	printf("%d\n", *int_data);
		free(int_data);
	}
	return 0;
}

/** Worker Keep Sending at the same time, do not try instensively **/
void*  worker_s(void *);
void*  worker_s(void *arg)
{
	int i = 0;
	int *int_data;
	while (i < total_put) {

		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i++;
		/*Enqueue*/

		while (lfqueue_enq(&myq, int_data)) {
			// printf("ENQ FULL?\n");
		}

	}
	return 0;
}

/** Worker Send And Consume at the same time **/
void*  worker_sc(void *);
void*  worker_sc(void *arg)
{
	int i = 0;
	int *int_data;
	while (i < total_put) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i++;
		/*Enqueue*/
		while (lfqueue_enq(&myq, int_data)) {
			// printf("ENQ FULL?\n");
		}

		/*Dequeue*/
		while ((int_data = lfqueue_deq(&myq)) == NULL) {
			// printf("DEQ EMPTY? %zu\n", lfqueue_size(&myq));
		}
		// printf("%d\n", *int_data);
		free(int_data);

	}
	return 0;
}

#define join_threads \
for (i = 0; i < nthreads; i++)\
pthread_join(threads[i], NULL);\
printf("current size= %d\n", (int) lfqueue_size(&myq) )

#define detach_thread_and_loop \
for (i = 0; i < nthreads; i++)\
pthread_detach(threads[i]);\
while (lfqueue_size(&myq)) {\
sleep(2);\
printf("current size= %zu\n", lfqueue_size(&myq) );\
}

void multi_enq_deq(pthread_t *threads) {
	int i;
	for (i = 0; i < nthreads; i++) {
		pthread_create(threads + i, NULL, worker_sc, NULL);
	}

	join_threads;
	// detach_thread_and_loop;
}

void one_deq_and_multi_enq(pthread_t *threads) {
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_s, NULL);

	worker_c(NULL);

	join_threads;
	// detach_thread_and_loop;
}

void one_enq_and_multi_deq(pthread_t *threads) {
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_c, NULL);

	worker_s(NULL);

	//join_threads;
	detach_thread_and_loop;
}

int main(void)
{
	const int total_run = 24;
	int n;
	for (n = 0; n < total_run; n++) {
		printf("running count = %d\n", n);
		lfqueue_init(&myq, 16, 1);

		/* Spawn threads. */
		pthread_t threads[nthreads];
		printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
		printf("Total requests %d \n", total_put);
		gettimeofday(&tv1, NULL);

		 one_enq_and_multi_deq(threads);
		//one_deq_and_multi_enq(threads);
		// multi_enq_deq(threads);

		gettimeofday(&tv2, NULL);
		printf ("Total time = %f seconds\n",
		        (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
		        (double) (tv2.tv_sec - tv1.tv_sec));

		//getchar();
		assert ( 0 == lfqueue_size(&myq) && "Error, all queue should be consumed but not");

		lfqueue_destroy(&myq);
	}
	return 0;
}
