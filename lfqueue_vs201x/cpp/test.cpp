
#include "test.h"

namespace lf_queue {

	namespace test {

		//-----------------------------------------------------------------------------------------------
//		struct timeval  tv1, tv2;
#define total_put 50000
#define total_running_loop 50
		constexpr int nthreads = 4;
		int one_thread = 1;
		int nthreads_exited = 0;

		/// ------------------------------------------------------------------------------------------ 
		struct lfq_deletor_type {
			void operator ()(lf_queue::lfqueue_t* lfqueue) {
				lfqueue_destroy(lfqueue);
				delete lfqueue;
				lfqueue = nullptr;
			}
		};

		using lfq_queue_ptr = std::unique_ptr<lfqueue_t, lfq_deletor_type >;
		// global for this file
		static lfq_queue_ptr lfq_ptr_{ new lfqueue_t };

		using threads_arr_type = std::array< std::thread, nthreads >;

		using message_type = const char*;
		//-----------------------------------------------------------------------------------------------

			// pass the reference to the array of std::thread's
		typedef void (*test_function)(threads_arr_type&);

		void one_enq_and_multi_deq(threads_arr_type&);
		void one_deq_and_multi_enq(threads_arr_type&);
		void multi_enq_deq(std::thread* threads);
		void* worker_sc(void*);
		void* worker_s(void*);
		void* worker_c(void*);
		void* worker_single_c(void*);

		/**Testing must**/
		void one_enq_and_multi_deq_must(threads_arr_type&);
		void one_deq_must_and_multi_enq(threads_arr_type&);
		void multi_enq_deq_must(std::thread* threads);
		void* worker_sc_must(void*);
		void* worker_s_must(void*);
		void* worker_c_must(void*);
		void* worker_single_c_must(void*);

		void running_test(test_function testfn);


		/// ---------------------------------------------------------------------------------------------

		void* worker_c_must(void* arg) {
			int multiplier_{ 1 };
			if (arg) {
				multiplier_ = int(*((int*)arg));
			}
			int total_loop = total_put * multiplier_;
			while ( --total_loop) {
				/*Dequeue*/
				message_type data_ = (message_type)lfqueue_deq_must(lfq_ptr_.get());

				if ( data_ ) printf("\n%s", data_);
			}
			return 0;
		}

		/// ---------------------------------------------------------------------------------------------
		void* worker_single_c_must(void* arg)
		{
			int multiplier_{ 1 };
			if (arg) {
				multiplier_ = int(*((int*)arg));
			}
			int total_loop = total_put * multiplier_;
			while (--total_loop) {
				/*Dequeue*/
				auto payload = (message_type)lfqueue_single_deq_must(lfq_ptr_.get());
				if (payload) printf("\n%s", payload);
			}
			return 0;
		}
		/// ---------------------------------------------------------------------------------------------
		void* worker_sc_must(void* arg)
		{
			int multiplier_{ 1 };
			if (arg) {
				multiplier_ = int(*((int*)arg));
			}
			int total_loop = total_put * multiplier_;
			message_type payload = __FUNCSIG__;

			while ( --total_loop) {
				/*Enqueue*/
				while (lfqueue_enq(lfq_ptr_.get(), (void*)payload)) {
					printf("ENQ FULL?\n");
				}
				/*Dequeue*/
				message_type payload_received = (message_type)lfqueue_deq_must(lfq_ptr_.get());
				if (payload_received)
				  printf("\n%s", payload_received);
			}
			return 0;
		}
		/// ---------------------------------------------------------------------------------------------

		void* worker_c(void* arg) {
			int multiplier_{ 1 };
			if (arg) {
				multiplier_ = int(*((int*)arg));
			}
			int total_loop = total_put * multiplier_;
			while (--total_loop) {
				/*Dequeue*/
				message_type data_{};
				while (( data_ = (message_type)lfqueue_deq(lfq_ptr_.get())) == nullptr) {
					lfqueue_sleep(1);
				}
				//	printf("%s\n", data_);
			}
			return 0;
		}

		/// ---------------------------------------------------------------------------------------------
		void* worker_single_c(void* arg) {
			int multiplier_{ 1 };
			if (arg) {
				multiplier_ = int(*((int*)arg));
			}
			int total_loop = total_put * multiplier_;
			while (--total_loop) {
				/*Dequeue*/
				message_type data_{};
				while ((data_ = (message_type)lfqueue_single_deq(lfq_ptr_.get())) == nullptr) {
					lfqueue_sleep(1);
				}
				//	printf("%s\n", data_);
			}
			return 0;
		}

		/// ---------------------------------------------------------------------------------------------
		/** Worker Keep Sending at the same time, do not try instensively **/
		void* worker_s(void* arg)
		{
			int multiplier_{ 1 };

			if (arg) {
				multiplier_ = int(*((int*)arg));
			}

			static message_type payload = "PAYLOAD";
			int walker_ = 0;
			int total_loop = total_put * multiplier_;
			while (walker_++ < total_loop) {

				/*Enqueue*/
				while (lfqueue_enq(lfq_ptr_.get(), &payload))
				{
					// printf("ENQ FULL?\n");
				}
			}
			// __sync_add_and_fetch(&nthreads_exited, 1);
			return 0;
		}

		/** Worker Send And Consume at the same time **/
		void* worker_sc(void* arg)
		{
			int i = 0;
			int* int_data;
			while (i < total_put) {
				int_data = (int*)malloc(sizeof(int));
				assert(int_data != NULL);
				*int_data = i++;
				/*Enqueue*/
				while (lfqueue_enq(lfq_ptr_, int_data)) {
					printf("ENQ FULL?\n");
				}

				/*Dequeue*/
				while ((int_data = lfqueue_deq(lfq_ptr_)) == NULL) {
					lfqueue_sleep(1);
				}
				// printf("%d\n", *int_data);
				free(int_data);
			}
			__sync_add_and_fetch(&nthreads_exited, 1);
			return 0;
		}

#if 0
#define join_threads \
for (i = 0; i < nthreads; i++) {\
pthread_join(threads[i], NULL); \
}

#define detach_thread_and_loop \
for (i = 0; i < nthreads; i++)\
pthread_detach(threads[i]);\
while ( nthreads_exited < nthreads ) \
	lfqueue_sleep(10);\
if(lfqueue_size(lfq_ptr_ ) != 0){\
lfqueue_sleep(10);\
}
#endif // 0

		/// --------------------------------------------------------------------------------------------
		/// functions given to running_test()
		/// --------------------------------------------------------------------------------------------

		void multi_enq_deq(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "multi_enq_deq");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_sc);
			}

			for (auto& thr_ : threads) {
				thr_.join();
			}
		}

		void one_deq_and_multi_enq(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "one_deq_and_multi_enq");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_s);
			}

			worker_single_c(threads);

			for (auto& thr_ : threads) {
				thr_.join();
			}
		}

		void one_enq_and_multi_deq(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "one_enq_and_multi_deq");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_c);
			}

			worker_s(threads);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
			for (auto& thr_ : threads) {
				thr_.join();
			}
#pragma GCC diagnostic pop

		}


		void one_deq_must_and_multi_enq(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "one_deq_must_and_multi_enq");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_s);
			}

			worker_single_c_must(nullptr);

			for (auto& thr_ : threads) {
				thr_.join();
			}
		}

		void one_enq_and_multi_deq_must(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "one_enq_and_multi_deq_must");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_c_must);
			}

			worker_s(nullptr);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
			for (auto& thr_ : threads) {
				thr_.join();
			}
#pragma GCC diagnostic pop

		}

		void multi_enq_deq_must(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "multi_enq_deq_must");
			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_sc_must);
			}
			for (auto& thr_ : threads) {
				thr_.join();
			}
		}

		/// --------------------------------------------------------------------------------------------

		void running_test(test_function testfn) {
			int n;

			for (n = 0; n < total_running_loop; n++) {
				printf("Running loop %d or %d ", n, total_running_loop);
				nthreads_exited = 0;

				/* Spawn N threads. */
				threads_arr_type threads_arr_{ {} };

				printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
				printf("Total requests %d \n", total_put);

				timer timer_{};
				testfn(threads_arr_);

				printf("Total time: %f seconds\n", as_buffer(timer_, timer::kind::second).data());

				lfqueue_sleep(10);
				assert(0 == lfqueue_size(lfq_ptr_.get()) && "Error, all queue should be consumed but is not");
			}
		}

	} // namespace test

} // lf_queue

/// ------------------------------------------------------------------------------------------ 
int main(const int, const char**)
{
	using namespace lf_queue;
	using namespace lf_queue::test;

	if (lfqueue_init(lfq_ptr_.get()) == -1)
	{
		perror(__FILE__ " -- lfqueue_init() has failed ");
		return  EXIT_FAILURE;
	}

	running_test(one_enq_and_multi_deq);
	running_test(one_enq_and_multi_deq_must);

	running_test(one_deq_and_multi_enq);
	running_test(one_deq_must_and_multi_enq);

	running_test(multi_enq_deq);
	running_test(multi_enq_deq_must);


	printf("All Tests have passed!\n");

	return EXIT_SUCCESS;
}

