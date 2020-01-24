
#include "test.h"

namespace lf_queue {

	namespace test {

		/// ------------------------------------------------------------------------------------------ 
		constexpr auto total_put = 50000U;
		constexpr auto total_running_loop = 50U;
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
		void multi_enq_deq(threads_arr_type&);
		
		void* worker_sender_consumer(void*);
		void* worker_sender(void*);
		void* worker_consumer(void*);
		void* worker_single_c(void*);

		/**Testing must**/
		void one_enq_and_multi_deq_must(threads_arr_type&);
		void one_deq_must_and_multi_enq(threads_arr_type&);
		void multi_enq_deq_must(threads_arr_type&);

		void* worker_sc_must(void*);
		// void* worker_s_must(void*);
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

				if (payload_received) printf("\n" __FUNCSIG__  " -- received -- %s\n", payload_received);
			}
			return 0;
		}
		/// ---------------------------------------------------------------------------------------------

		void* worker_consumer(void* arg) {

			int total_loop = total_put ;
			printf("\n");
			while (--total_loop) {
				/*Dequeue*/
				message_type data_{};
				while (( data_ = (message_type)lfqueue_deq(lfq_ptr_.get())) == nullptr) {
					lfqueue_sleep(1);
				}
				if(data_) printf( "\r worker_consumer <-- received --| %32s [%6d]",  data_, total_loop);
			}
			printf("\n");
			return 0;
		}

		/// ---------------------------------------------------------------------------------------------
		void* worker_single_c(void* arg) {
			int total_loop = total_put ;
			while (--total_loop) {
				/*Dequeue*/
				message_type data_{};
				while ((data_ = (message_type)lfqueue_single_deq(lfq_ptr_.get())) == nullptr) {
					lfqueue_sleep(1);
				}
				if (data_) printf("\nworker_single_c-- received -- %s\n", data_);
			}
			return 0;
		}

		/// ---------------------------------------------------------------------------------------------
		/** Worker Keep Sending at the same time, do not try instensively **/
		void* worker_sender(void* arg)
		{
			int total_loop = total_put ;

			static message_type payload = " worker_sender message "  ;

			while (--total_loop) {

				/*Enqueue*/
				while (lfqueue_enq(lfq_ptr_.get(), &payload))
				{
					printf( " worker_sender() -- ENQ FULL?\n");
				}

				lfqueue_sleep(1); // sleep one millisecond
			}
			return 0;
		}

		/// ---------------------------------------------------------------------------------------------
		/** Worker Send And Consume at the same time **/
		void* worker_sender_consumer(void* arg)
		{
			int total_loop = total_put ;
			static message_type payload = "worker_sender_consumer message";
			while (--total_loop) {
				/*Enqueue*/
				while (lfqueue_enq(lfq_ptr_.get(), (void*)payload)) {
					printf("ENQ FULL?\n");
				}
				/*Dequeue*/
				while ((payload = (message_type)lfqueue_deq(lfq_ptr_.get())) == nullptr) {
					lfqueue_sleep(1);
				}
				if (payload) printf("\rworker_sender_consumer -- received -- %30s [%6d]", payload, total_loop);
			}
			return 0;
		}

		/// --------------------------------------------------------------------------------------------
		/// functions given to running_test()
		/// --------------------------------------------------------------------------------------------

		void multi_enq_deq(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "multi_enq_deq");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_sender_consumer, nullptr);
			}

			for (auto& thr_ : threads) {
				thr_.join();
			}
		}

		void one_deq_and_multi_enq(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "one_deq_and_multi_enq");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_sender,nullptr);
			}

			worker_single_c(nullptr);

			for (auto& thr_ : threads) {
				thr_.join();
			}
		}
		///-------------------------------------------------------------------------------------------
		void one_enq_and_multi_deq(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "one_enq_and_multi_deq");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_consumer,nullptr);
			}

			worker_sender(nullptr);

			for (auto& thr_ : threads) 
			{
				while (false == thr_.joinable())
				{
					lfqueue_sleep(1);
				}
				thr_.join();
			}
		}


		void one_deq_must_and_multi_enq(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "one_deq_must_and_multi_enq");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_sender, nullptr);
			}

			worker_single_c_must(nullptr);

			for (auto& thr_ : threads) {
				thr_.join();
			}
		}

		void one_enq_and_multi_deq_must(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "one_enq_and_multi_deq_must");

			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_c_must, nullptr);
			}

			worker_sender(nullptr);

			for (auto& thr_ : threads) {
				thr_.join();
			}
		}

		void multi_enq_deq_must(threads_arr_type& threads) {
			printf("-----------%s---------------\n", "multi_enq_deq_must");
			for (auto& thr_ : threads) {
				thr_ = std::thread(worker_sc_must, nullptr );
			}
			for (auto& thr_ : threads) {
				thr_.join();
			}
		}

		/// --------------------------------------------------------------------------------------------

		void running_test(test_function testfn) {

			for (int n = 0; n < total_running_loop; n++) {
				printf("Running loop %4d of %4d ", n, total_running_loop);

				/* Spawn N threads. */
				threads_arr_type threads_arr_{ {} };

				printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
				printf("Total requests %d \n", total_put);

				timer timer_{};
				testfn(threads_arr_);

				printf("Total time: %s seconds\n", as_buffer(timer_, timer::kind::second).data());

				lfqueue_sleep(10);
				assert(0 == lfqueue_size(lfq_ptr_.get()) && "Error, all queues should be consumed but they are not");
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
	/*
	running_test(one_enq_and_multi_deq_must);
	running_test(one_deq_and_multi_enq);
	running_test(one_deq_must_and_multi_enq);
	running_test(multi_enq_deq);
	running_test(multi_enq_deq_must);
	*/

	printf("All Tests have passed!\n");

	return EXIT_SUCCESS;
}

