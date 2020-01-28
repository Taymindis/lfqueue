

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "../../lfqueue.h"

#ifndef LFQ_WINDOWS
#error This is Windows only code
#endif

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h> /* _beginthread, _endthread */

/// --------------------------------------------------------------------------------------------
static lfqueue_t the_queue;
#define MAX_THREADS 42
static const int total_put = 50000;

/// origin is a "strong type" (c) 2019 dbj@dbj.org
///
/// void fun(origin);
///
/// is infinitely better than
///
/// void fun ( char * );
///
/// calling is even better
///
/// fun((origin){"Producer"});
///
/// almost like named argument, but not, because
/// the name required is a type
/// thus type and value cleraly stated in a call
/// better than C++ variant:
///
/// C++
/// fun({"Producer"}); // what is the type passed?
///
typedef struct origin_tag
{
	const char *val;
} origin;
/// --------------------------------------------------------------------------------------------
/// one can pass objects on stack to threads, but stack is shared by default between threads
/// so for clear confusion free messaging use heap

char *make_name_(unsigned id_, origin origin_)
{
	assert(origin_.val);
	char *retval = (char *)calloc(0xF, sizeof(char));
	assert(retval);
	int count = snprintf(retval, 0xF, "%s: %3d", origin_.val, id_);
	assert(!(count < 0));
	return retval;
}
void free_name_(char *pn_)
{
	free(pn_);
	pn_ = 0;
}
char *print_name_(void *vp_, origin origin_)
{
	assert(vp_);
	assert(origin_.val);
	char *pn_ = (char *)vp_;
	printf("\n%16s [%16s] has started", origin_.val, pn_);
	fflush(0);
	return pn_;
}
char *make_message_(char *producer_name_)
{
	assert(producer_name_);
	char *retval = (char *)calloc(0xFF, sizeof(char));
	assert(retval);
	int count = snprintf(retval, 0xFF, "Message from: %s", producer_name_);
	assert(!(count < 0));
	return retval;
}
void free_message_(char *pn_)
{
	free(pn_);
	pn_ = 0;
}

/// --------------------------------------------------------------------------------------------
unsigned __stdcall producer(void *);
unsigned __stdcall producer(void *arg)
{
	// origin producer_ = {"Producer"};
	/* print to terminal window */
	char *name_ = print_name_(arg, (origin){""});
	/* make the message to be sent */
	char *message_ = make_message_(name_);
	/*
	Enqueue, consumer is responsible freeing the message
	*/
	while (lfqueue_enq(&the_queue, message_))
	{
		perror("Q FULL for ?\n");
	}
	free_name_(name_);
	lfqueue_sleep(1);
	return 0;
}
/// --------------------------------------------------------------------------------------------
/// running "for ever" in his thread
unsigned __stdcall consumer(void *);
unsigned __stdcall consumer(void *arg)
{
	/* print to terminal window */
	char *name_ = print_name_(arg, (origin){"Consumer"});

	while (1)
	{
		/*Dequeue
		This call is only applicable when a single thread consumes messages 
		*/
		char *message_ = lfqueue_single_deq_must(&the_queue);

		if (message_)
		{
			printf("\nConsumer has received: [%s]", message_);
			free_name_(message_);
		}
		else
		{
			printf("\nConsumer should not be here..");
		}
		fflush(0);
		lfqueue_sleep(1);
	}
	return 0;
}

/*----------------------------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	//const static int MAX_THREADS = 2;//sysconf(_SC_NPROCESSORS_ONLN); // Linux

	static const int LOOP_SIZE_ = 1;

	if (lfqueue_init(&the_queue) == -1)
	{
		perror("LF QUEUE initialization has failed.");
		return EXIT_FAILURE;
	}

	for (int n = 0; n < LOOP_SIZE_; n++)
	{

		/* the threads */
		HANDLE threads[MAX_THREADS] = {0};

		unsigned consumer_thread_id = 0;

		HANDLE consumer_thread = (HANDLE)_beginthreadex(NULL, 0, consumer,
														make_name_(consumer_thread_id, (origin){"Consumer"}), CREATE_SUSPENDED, NULL);

		/* begin producers, each on a separate thread */
		int i = 1;
		do
		{
			threads[i] = (HANDLE)_beginthreadex(NULL, 0, producer, make_name_(i, (origin){"Producer"}), 0, NULL);
			i++;
		} while (i < MAX_THREADS);

		/// resume the single consumer
		ResumeThread(consumer_thread);
		/// wait 5 seconds for consumer
		WaitForSingleObject(consumer_thread, 1000 * 5 /* 5 seconds */);
		CloseHandle(consumer_thread);

		/// wait for  producers
		WaitForMultipleObjects(MAX_THREADS, threads, TRUE, INFINITE);

		// Close event handles
		for (int i = 0; i < MAX_THREADS; i++)
			CloseHandle(threads[i]);

	} /* main loop */

	fflush(0);

	while (0 != lfqueue_size(&the_queue))
	{
		// perror("All queues should be consumed but they are not");
		lfqueue_sleep(1);
		// return EXIT_FAILURE;
	}

	lfqueue_destroy(&the_queue);

	printf("\nDone, hit any key you choose ... ");
	fflush(0);
	getch();

	return EXIT_SUCCESS;
}
