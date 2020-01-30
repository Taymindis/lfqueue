
/// MSVC, C++17
/// one consumer, 1..N producers

#include <thread>
#include <future>
#include <vector>
#include <cstdio>
#include "../../lfqueue.h" // namespace lf_queue
#include "../common.h"     // namespace common

using namespace std;
#define MAX_PRODUCER_THREADS 2U
using namespace common;
using namespace ::lf_queue;
using namespace std::chrono_literals;

/// Prototypes
/// __declspec(dllimport)
/// extern "C" __declspec(dllimport) int __stdcall CloseHandle(void *hObject);
/// #pragma comment(lib, "Kernel32.lib")
///--------------------------------------------------
static lf_queue::lfqueue_t the_queue;

using producers_sequence = vector<future<void>>;
///--------------------------------------------------
void __stdcall producer(unsigned producer_threads_count)
{
    using namespace common;
    using namespace ::lf_queue;
    // not how we take the copy not the reference
    /* using C API provokes cludges */
    char buf[3] = {0};
    snprintf(buf, 3, "%d", producer_threads_count);
    /* 1: returns first arg cast to char*, does *not* allocate anything  */
    char *name_ = print_name_((void *)buf, {"Producer"});
    printf(" in thread %d\n", (int)GetCurrentThreadId());
    /* 2: make the message to be sent */
    char *message_ = make_message_(name_);
    /* 3: Enqueue, consumer is responsible freeing the message	*/
    while (lfqueue_enq(&the_queue, message_))
    {
        this_thread::yield();
    }
    // producer sends one message and exits
    this_thread::yield();
}

///--------------------------------------------------
void __stdcall consumer()
{
    using namespace common;
    using namespace ::lf_queue;
    const char *consumer_id_ = "99";
    /* using C API provokes cludges */
    /* 1: returns arg cast to char*, does not allocate anything  */
    char *name_ = print_name_((void *)consumer_id_, {"Consumer"});
    printf(" from thread %d\n", (int)GetCurrentThreadId());
    unsigned number_of_messages_received = 0;
    // we will loop forever if this is required
    while (true)
    {
        this_thread::yield();

        /*Dequeue -- This call is only applicable when a single thread consumes messages */
        char *message_ = (char *)lfqueue_single_deq_must(&the_queue);

        if (message_)
        {
            printf("\nConsumer has received: [%s]", message_);
            /* made on the heap by producer, must free it */
            free_name_(message_);

            // Here we decide what is the exiting criteria for the consumer
            // for example a special (aka control) message
            // our logic here is very simple, we exit when number
            // of messages equals the number of prodcers threads
            // each of them send a single message and exits
            if (++number_of_messages_received == MAX_PRODUCER_THREADS)
            {
                printf("\nConsumer has recieved %d messages from %d senders, it is exiting\n",
                       number_of_messages_received, MAX_PRODUCER_THREADS);
                break; // out
            }
        }
        else
        {
            printf("\nConsumer should not be here..");
        }
        //                this_thread::sleep_for(1ns); // 1 nano seconds
    }
    fflush(0);
}
///--------------------------------------------------
int worker(int, char **);
int main(int argc, char **argv)
{
    try
    {
        worker(argc, argv);
    }
    catch (std::future_error const &err)
    {
        printf("\nstd future_error: %s", err.what());
    }
    catch (std::system_error &syserr_)
    {
        printf("\nstd exception: %s", syserr_.what());
    }
    catch (std::exception &ex_)
    {
        printf("\nstd exception: %s", ex_.what());
    }
    catch (...)
    {
        printf("\nUnknown exception");
    }
    return EXIT_SUCCESS;
}
///--------------------------------------------------
static int worker(int, char **)
{

    if (lfqueue_init(&the_queue) == -1)
    {
        perror("LF QUEUE initialization has failed.");
        return EXIT_FAILURE;
    }

    unsigned producer_threads_count = MAX_PRODUCER_THREADS;

    producers_sequence producers{0};
    /// it is important to realize things are NOT happening in-order
    /// in multi-threading apps.
    /// For example single consumer bellow might start before
    /// any of the producers
    /// the key in makin things the way you want, is synchronization
    do
    {
        producers.push_back(std::async(producer, producer_threads_count));
    } while (--producer_threads_count);

    /// a single consumer
    thread consumerT{consumer}; // consumer ctor

    /// IF we want consumer thread to be 'detached'
    /// thus we will keep it's handle so we can close
    /// it in a controlled manner later on
    /// std::thread::native_handle_type

    // auto consumer_handle = consumer.native_handle();
    consumerT.detach();
    /*
    After this "just leave it" .. do not join 
    or close handle!
    */

    // printf("\nMain thread sleeping for 5 seconds...");
    // using namespace std::chrono_literals;
    // this_thread::sleep_for(5s);

    printf("\n\n");
    fflush(0);
    /// join and stop the producers
    unsigned count_ = 1;
    for (auto &fut_ : producers)
    {
        printf("\rWaiting for producer %3d future", count_++);
        if (fut_.valid())
            fut_.wait();
        this_thread::yield();
    }

    /// will throw the exception if not joinable
    // while (!consumer.joinable())
    // {
    //     printf("\rWaiting for consumer to become joinable...");
    //     this_thread::yield();
    // }
    // printf("\nConsumer is joining...");
    // consumer.join();
    /// this shuts down the consumer thread
    // CloseHandle(consumer_handle);

    fflush(0);
    while (0 != lfqueue_size(&the_queue))
    {
        // perror("All queues should be consumed but they are not");
        lfqueue_sleep(1);
        // return EXIT_FAILURE;
    }
    lfqueue_destroy(&the_queue);
    fflush(0);
    return EXIT_SUCCESS;
} // worker