#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
#include <stdio.h>
#include "lfqueue.h"


typedef void (*test_function)(pthread_t*);

void one_enq_and_multi_deq(pthread_t *threads);
void one_deq_and_multi_enq(pthread_t *threads);
void multi_enq_deq(pthread_t *threads);
void*  worker_sc(void *);
void*  worker_s(void *);
void*  worker_c(void *);
void*  worker_single_c(void *);

/**Testing must**/
void one_enq_and_multi_deq_must(pthread_t *threads);
void one_deq_must_and_multi_enq(pthread_t *threads);
void multi_enq_deq_must(pthread_t *threads);
void*  worker_sc_must(void *);
void*  worker_s_must(void *);
void*  worker_c_must(void *);
void*  worker_single_c_must(void *);

void running_test(test_function testfn);

struct timeval  tv1, tv2;
#define total_put 50000
#define total_running_loop 50
int nthreads = 4;
int one_thread = 1;
int nthreads_exited = 0;
lfqueue_t *myq;



void*  worker_c_must(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		/*Dequeue*/
		int_data = lfqueue_deq_must(myq);
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_single_c_must(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		/*Dequeue*/
		int_data = lfqueue_single_deq_must(myq);
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_sc_must(void *arg)
{
	int i = 0;
	int *int_data;
	while (i < total_put) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i++;
		/*Enqueue*/
		while (lfqueue_enq(myq, int_data)) {
			printf("ENQ FULL?\n");
		}

		/*Dequeue*/
		int_data = lfqueue_deq_must(myq);
		// printf("%d\n", *int_data);
		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_c(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		/*Dequeue*/
		while ((int_data = lfqueue_deq(myq)) == NULL) {
			lfqueue_sleep(1);
		}
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_single_c(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		/*Dequeue*/
		while ((int_data = lfqueue_single_deq(myq)) == NULL) {
			lfqueue_sleep(1);
		}
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

/** Worker Keep Sending at the same time, do not try instensively **/
void*  worker_s(void *arg)
{
	int i = 0, *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i;
		/*Enqueue*/

		while (lfqueue_enq(myq, int_data)) {
			// printf("ENQ FULL?\n");
		}
	}
	// __sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

/** Worker Send And Consume at the same time **/
void*  worker_sc(void *arg)
{
	int i = 0;
	int *int_data;
	while (i < total_put) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i++;
		/*Enqueue*/
		while (lfqueue_enq(myq, int_data)) {
			printf("ENQ FULL?\n");
		}

		/*Dequeue*/
		while ((int_data = lfqueue_deq(myq)) == NULL) {
			lfqueue_sleep(1);
		}
		// printf("%d\n", *int_data);
		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

#define join_threads \
for (i = 0; i < nthreads; i++) {\
pthread_join(threads[i], NULL); \
}

#define detach_thread_and_loop \
for (i = 0; i < nthreads; i++)\
pthread_detach(threads[i]);\
while ( nthreads_exited < nthreads ) \
	lfqueue_sleep(10);\
if(lfqueue_size(myq) != 0){\
lfqueue_sleep(10);\
}

void multi_enq_deq(pthread_t *threads) {
	printf("-----------%s---------------\n", "multi_enq_deq");
	int i;
	for (i = 0; i < nthreads; i++) {
		pthread_create(threads + i, NULL, worker_sc, NULL);
	}

	join_threads;
	// detach_thread_and_loop;
}

void one_deq_and_multi_enq(pthread_t *threads) {
	printf("-----------%s---------------\n", "one_deq_and_multi_enq");
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_s, &one_thread);

	worker_single_c(&nthreads);

	join_threads;
	// detach_thread_and_loop;
}

void one_enq_and_multi_deq(pthread_t *threads) {
	printf("-----------%s---------------\n", "one_enq_and_multi_deq");
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_c, &one_thread);

	worker_s(&nthreads);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	detach_thread_and_loop;
#pragma GCC diagnostic pop

}


void one_deq_must_and_multi_enq(pthread_t *threads) {
	printf("-----------%s---------------\n", "one_deq_must_and_multi_enq");
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_s, &one_thread);

	worker_single_c_must(&nthreads);

	join_threads;
	// detach_thread_and_loop;
}

void one_enq_and_multi_deq_must(pthread_t *threads) {
	printf("-----------%s---------------\n", "one_enq_and_multi_deq_must");
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_c_must, &one_thread);

	worker_s(&nthreads);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	detach_thread_and_loop;
#pragma GCC diagnostic pop

}

void multi_enq_deq_must(pthread_t *threads) {
	printf("-----------%s---------------\n", "multi_enq_deq_must");
	int i;
	for (i = 0; i < nthreads; i++) {
		pthread_create(threads + i, NULL, worker_sc_must, NULL);
	}

	join_threads;
	// detach_thread_and_loop;
}

void running_test(test_function testfn) {
	int n;
	for (n = 0; n < total_running_loop; n++) {
		printf("Current running at =%d, ", n);
		nthreads_exited = 0;
		/* Spawn threads. */
		pthread_t threads[nthreads];
		printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
		printf("Total requests %d \n", total_put);
		gettimeofday(&tv1, NULL);

		testfn(threads);
		// one_enq_and_multi_deq(threads);

		//one_deq_and_multi_enq(threads);
		// multi_enq_deq(threads);
		// worker_s(&ri);
		// worker_c(&ri);

		gettimeofday(&tv2, NULL);
		printf ("Total time = %f seconds\n",
		        (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
		        (double) (tv2.tv_sec - tv1.tv_sec));

		lfqueue_sleep(10);
		assert ( 0 == lfqueue_size(myq) && "Error, all queue should be consumed but not");
	}
}

int main(void) {
	myq = malloc(sizeof	(lfqueue_t));
	if (lfqueue_init(myq) == -1)
		return -1;

	running_test(one_enq_and_multi_deq);
	running_test(one_enq_and_multi_deq_must);

	running_test(one_deq_and_multi_enq);
	running_test(one_deq_must_and_multi_enq);

	running_test(multi_enq_deq);
	running_test(multi_enq_deq_must);


	lfqueue_destroy(myq);
	// sleep(3);
	free(myq);

	printf("Test Pass!\n");

	return 0;
}

