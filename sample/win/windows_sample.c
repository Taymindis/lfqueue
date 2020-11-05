

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "../../lfqueue.h"
#include "../common.h"

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

/// --------------------------------------------------------------------------------------------
unsigned __stdcall producer(void *);
unsigned __stdcall producer(void *arg)
{
char * name_arg = (char *)arg;
	/* make the message to be sent */
	Common_Message *message_ = make_message_((Common_Origin){name_arg});
	/*
	Enqueue, consumer is responsible freeing the message
	*/
	while (lfqueue_enq(&the_queue, message_))
	{
		// perror("Q FULL for ?\n");
		lfqueue_sleep(1);
	}
	// NO! --> free_name_(name_);
	lfqueue_sleep(1);
	return 0;
}
/// --------------------------------------------------------------------------------------------
/// running "for ever" in his thread
/// name is coming as argument
unsigned __stdcall consumer(void *);
unsigned __stdcall consumer(void *arg)
{
printf("\nStarted: %s",  (char*)arg );

while (1)
{
	/*Dequeue
		This call is only applicable when a single thread consumes messages 
		*/
	Common_Message *message_ = lfqueue_single_deq_must(&the_queue);

	if (message_)
	{
		printf("\nConsumer has received: %-55s", message_->val);
		free_message_(message_);
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

		unsigned consumer_thread_idx_ = MAX_THREADS + 100;

		HANDLE consumer_thread = (HANDLE)_beginthreadex(
			NULL, 0, consumer,
			/* thread data is the name we generate for it */
			make_name_(
				(Common_Id){consumer_thread_idx_},
				(Common_Origin){"Consumer"})
				.val,
			CREATE_SUSPENDED, NULL);

		/* begin producers, each on a separate thread */
		int i = 0;
		do
		{
			threads[i] = (HANDLE)_beginthreadex(
				NULL, 0, producer,
				/* thread data is the name we generate for it */
				make_name_(
					(Common_Id){i}, (Common_Origin){"Producer"})
					.val,
				0, NULL);
			i++;
		} while (i < MAX_THREADS);

		/// resume the single consumer
		ResumeThread(consumer_thread);
		/// wait 5 seconds for consumer
		WaitForSingleObject(consumer_thread, 1000 /* 1 second */);
		/// shutdown consumer thread
		CloseHandle(consumer_thread);

		/// wait for  producers
		WaitForMultipleObjects(MAX_THREADS, threads, TRUE, INFINITE);

		// Close producers threads
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

	printf("\nDone, hit any key you choose to hit today ... \n");
	fflush(0);
	getch();

	return EXIT_SUCCESS;
}
