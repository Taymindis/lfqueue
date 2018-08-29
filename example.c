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
lfqueue_t results;

#define total_put 50000

void *worker(void *);
void *worker(void *arg)
{
    int i = 0;
    int *int_data;
    while (i < total_put) {
        int_data = (int*) malloc(sizeof(int));
        assert(int_data != NULL);
        *int_data = i++;
        /*Enqueue*/

        while (lfqueue_enq(&results, int_data)) {
        	printf("ENQ FULL?\n");
		}

        /*Dequeue*/
        while  ( (int_data = lfqueue_deq(&results)) == NULL) {
            printf("DEQ EMPTY?\n");
        }
        free(int_data);
    }
    return NULL;
}

#define join_threads \
for (i = 0; i < nthreads; i++)\
pthread_join(threads[i], NULL);\
printf("current size= %d\n", (int) lfqueue_size(&results) )

#define detach_thread_and_loop \
for (i = 0; i < nthreads; i++)\
pthread_detach(threads[i]);\
while (1) {\
sleep(2);\
printf("current size= %zu\n", lfqueue_size(&results) );\
}

int main(void)
{
    int nthreads = 16;//sysconf(_SC_NPROCESSORS_ONLN); // Linux
    int i;

    lfqueue_init(&results, total_put);

    /* Spawn threads. */
    pthread_t threads[nthreads];
    printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
    printf("Total requests %d \n", total_put);
    gettimeofday(&tv1, NULL);
    for (i = 0; i < nthreads; i++)
        pthread_create(threads + i, NULL, worker, NULL);

    join_threads;
    // detach_thread_and_loop;

    gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
            (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
            (double) (tv2.tv_sec - tv1.tv_sec));

    assert ( 0 == lfqueue_size(&results) && "Error, all queue should be consumed but not");
    
    lfqueue_destroy(&results);
    return 0;
}

