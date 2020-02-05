
/// MSVC, C++17
/// one consumer, 1..N futures
/// it is important to realize things are NOT happening in-order
/// in multi-threading apps.
/// For example single consumer bellow might start before
/// any of the futures
/// the key in makin things the way you want, is synchronization
#include <future>
#include <vector>
#include <cstdio>
#include "../../lfqueue.h" // namespace lf_queue
#include "../common.h"     // namespace common

using namespace std;
using namespace common;
using namespace ::lf_queue;
using namespace std::chrono_literals;
#define MAX_PRODUCER_THREADS 255U

/// if required Prototype CloseHandle()
/// instead of including the whole windows.h for this one function
/// __declspec(dllimport)
/// extern "C" __declspec(dllimport) int __stdcall CloseHandle(void *hObject);
/// #pragma comment(lib, "Kernel32.lib")
///--------------------------------------------------
static lf_queue::lfqueue_t the_queue;

using futures_sequence = vector<future<void>>;
///--------------------------------------------------
void __stdcall producer(unsigned producer_threads_count)
{
    using namespace common;
    using namespace ::lf_queue;
    /* using C API provokes cludges */
    //char buf[3] = {0};
    //snprintf(buf, 3, "%d", producer_threads_count);
    /* 1: returns first arg cast to char*, does *not* allocate anything  */
    Common_Name name_ = make_name_({producer_threads_count}, {"Producer"});
    /* 2: make the message to be sent */
    Common_Message * message_ = make_message_({name_.val});
    /* 3: Enqueue, consumer is responsible freeing the message	*/
    while (lfqueue_enq(&the_queue, message_))
    {
        this_thread::yield();
    }
    // producer sends one message and exits
    // but before doing so it give other threads a bit more time
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
    Common_Name name_ = make_name_({99}, {"Consumer"});
    unsigned number_of_messages_received = 0;
    // we will loop forever if this is required
    while (true)
    {
        this_thread::yield();

        /*Dequeue -- This call is only applicable when a single thread consumes messages */
        Common_Message *message_ = 
        (Common_Message *)lfqueue_single_deq_must(&the_queue);

        if (message_)
        {
            /* made on the heap by producer, must free it */
            free_message_(message_);

            // Here we decide what is the exiting criteria for the consumer
            // for example a special (aka control) message?
            // Our logic here is very simple, we exit when number
            // of messages equals the number of futures threads
            // each of them sends a single message and exits
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
    }
    fflush(0);
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

    futures_sequence futures{0};

    do
    {
        futures.push_back(std::async(producer, producer_threads_count));
    } while (--producer_threads_count);

    /// a single consumer is added last
    /// NOTE: that does not guarantee it will start last
    futures.push_back(std::async(consumer));

    printf("\n");
    fflush(0);
    /// wait for all the futures
    unsigned count_ = 1;
    for (auto &fut_ : futures)
    {
    again:
        if (fut_.valid())
            fut_.wait();
        else
        {
            /// you never know ...
            printf("\rWaiting for the future %3d, hit CTRL+C if app is stuck here", count_++);
            /// give them some time
            this_thread::yield();
            goto again;
        }
    }

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

///--------------------------------------------------
/// we can build with or without exceptions enabled
///
int main(int argc, char **argv)
{
#if _HAS_EXCEPTIONS
    try
    {
#endif // _HAS_EXCEPTIONS
        return worker(argc, argv);
#if _HAS_EXCEPTIONS
    }
    catch (std::future_error const &err)
    {
        printf("\nstd future_error: %s", err.what());
    }
    catch (std::system_error &syserr_)
    {
        printf("\nstd system_error: %s", syserr_.what());
    }
    catch (std::exception &ex_)
    {
        printf("\nstd exception: %s", ex_.what());
    }
    catch (...)
    {
        printf("\nUnknown exception");
    }
#endif // _HAS_EXCEPTIONS
    return EXIT_SUCCESS;
}
