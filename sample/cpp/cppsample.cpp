
/// MSVC, C++17
/// one consumer, 1..N producers

#include <thread>
#include <vector>
#include <cstdio>
#include "../../lfqueue.h" // namespace lf_queue
#include "../common.h"     // namespace common

using namespace std;

/// Prototypes
/// __declspec(dllimport)
extern "C" __declspec(dllimport) int __stdcall CloseHandle(void *hObject);

/// #pragma comment(lib, "Kernel32.lib")
///--------------------------------------------------
static lf_queue::lfqueue_t the_queue;

using threads_sequence = vector<thread>;

int main(int, char **)
{
    using namespace common;
    using namespace ::lf_queue;
    unsigned producer_threads_count = 2U;

    if (lfqueue_init(&the_queue) == -1)
    {
        perror("LF QUEUE initialization has failed.");
        return EXIT_FAILURE;
    }
    threads_sequence producers{producer_threads_count};
    do
    {
        producers.push_back(
            thread([producer_threads_count] {
                /* using C API provokes cludges */
                char buf[3] = {0};
                snprintf(buf, 3, "%d", producer_threads_count);
                /* 1: returns first arg cast to char*, does *not* allocate anything  */
                char *name_ = print_name_((void *)buf, {"Producer"});
                printf(" from thread %d\n", (int)GetCurrentThreadId());
                printf("\nProducer [%s] sleeping for 5 seconds...", name_);
                using namespace std::chrono_literals;
                this_thread::sleep_for(5s);

                /* 2: make the message to be sent */
                char *message_ = make_message_(name_);
                /* 3: Enqueue, consumer is responsible freeing the message	*/
                while (lfqueue_enq(&the_queue, message_))
                {
                    lfqueue_sleep(1); // sleep if Q is full
                }
                return 0;
            }) // thread ctor
        );     // push_back
    } while (--producer_threads_count);
    /// a single consumer
    thread consumer{
        [&] {
            const char *consumer_id_ = "99";
            /* using C API provokes cludges */
            /* 1: returns arg cast to char*, does not allocate anything  */
            char *name_ = print_name_((void *)consumer_id_, {"Consumer"});
            printf(" from thread %d\n", (int)GetCurrentThreadId());
            printf("\nConsumer [%s] sleeping for 5 seconds...", name_);
            using namespace std::chrono_literals;
            this_thread::sleep_for(5s);
            // we will loop forever if this is required
            // while (true)
            // {
            /*Dequeue -- This call is only applicable when a single thread consumes messages */
            char *message_ = (char *)lfqueue_single_deq_must(&the_queue);

            if (message_)
            {
                printf("\nConsumer has received: [%s]", message_);
                /* made on the heap by producer */
                free_name_(message_);
            }
            else
            {
                printf("\nConsumer should not be here..");
            }
            fflush(0);
            this_thread::sleep_for(1ns); // 1 nano seconds
            // }
            return 0;
        }}; // consumer ctor

    /// IF we want consumer thread to be 'detached'
    /// thus we will keep it's handle so we can close
    /// it in a controlled manner later on
    /// std::thread::native_handle_type

    auto consumer_handle = consumer.native_handle();
    /*
    consumer.detach();
    After this "just leave it" .. do not join 
    or close hand!le
    */

    printf("\nMain thread sleeping for 5 seconds...");
    using namespace std::chrono_literals;
    this_thread::sleep_for(5s);

    /// join and stop the producers
    for (auto &thr_ : producers)
    {
        auto handle_ = thr_.native_handle();
        while (!thr_.joinable())
        {
            this_thread::sleep_for(1ns); // 1 nano sec
        }
        thr_.join();
        CloseHandle(handle_);
    }

    /// will throw the exception if not joinable
    while (!consumer.joinable())
    {
        this_thread::sleep_for(1ns); // 1 nano sec
    }
    consumer.join();
    /// this shuts down the consumer thread
    CloseHandle(consumer_handle);

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
} // main