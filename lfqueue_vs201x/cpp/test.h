#pragma once

/*
------------------------------------------------------------------------------------------------
CHECK FOR C++ VERSION
------------------------------------------------------------------------------------------------
Note: these should be intrinsic macros
*/

// this is MSVC STD LIB code
// it actually does not depend on C++20 __cplusplus
// which is yet undefined as of 2020 Q1

#if !defined(LFQ_HAS_CXX17) && !defined(LFQ_HAS_CXX20)

#if defined(_MSVC_LANG)
#define LFQ__STL_LANG _MSVC_LANG
#else
#define LFQ__STL_LANG __cplusplus
#endif

#if LFQ__STL_LANG > 201703L
#define LFQ_HAS_CXX17 1
#define LFQ_HAS_CXX20 1
#elif LFQ__STL_LANG > 201402L
#define LFQ_HAS_CXX17 1
#define LFQ_HAS_CXX20 0
#else // LFQ__STL_LANG <= 201402L
#define LFQ_HAS_CXX17 0
#define LFQ_HAS_CXX20 0
#endif // Use the value of LFQ__STL_LANG to define LFQ_HAS_CXX17 and \
       // LFQ_HAS_CXX20

// #undef LFQ__STL_LANG
#endif // !defined(LFQ_HAS_CXX17) && !defined(LFQ_HAS_CXX20)

#ifndef LFQ_HAS_CXX17
#error C++17 is required
#endif // LFQ_HAS_CXX17

 /*
 We include climits so that non intrinsic macros are avaiable too.
 For C++20 that should be the role of the <version> header
 http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0754r2.pdf

*/
/*
LINUX headers
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
*/
#include <array>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
// C API is in lf_queue namespace when consumed from C++
#include "../../lfqueue.h"

namespace lf_queue::test {

/*
Usage:

stopwatch::precise stopwatch{};

Note: all timings are integer types, no decimals

auto nanos_  = stopwatch.elapsed<unsigned int, std::chrono::nanoseconds>();
auto millis_ = stopwatch.elapsed<unsigned int, std::chrono::milliseconds>();
auto micros_ = stopwatch.elapsed<unsigned int, std::chrono::microseconds>();
auto secos_  = stopwatch.elapsed<unsigned int, std::chrono::seconds>();
*/
// https://wandbox.org/permlink/BHVUDSoZn1Cm8yzo
	namespace stopwatch
	{
		template <typename CLOCK = std::chrono::high_resolution_clock>
		class engine
		{
			const typename CLOCK::time_point start_point{};

		public:
			engine() : start_point(CLOCK::now())
			{
			}

			template <
				typename REP = typename CLOCK::duration::rep,
				typename UNITS = typename CLOCK::duration>
				REP elapsed() const
			{
				std::atomic_thread_fence(std::memory_order_relaxed);
				auto elapsed_ =
					std::chrono::duration_cast<UNITS>(CLOCK::now() - start_point).count();
				std::atomic_thread_fence(std::memory_order_relaxed);
				return static_cast<REP>(elapsed_);
			}
		};

		using precise = engine<>;
		using system = engine<std::chrono::system_clock>;
		using monotonic = engine<std::chrono::steady_clock>;
	} // namespace stopwatch

	///		times as decimals
	struct timer final
	{
		using buffer = std::array<char, 24>;
		using CLOCK = typename std::chrono::high_resolution_clock;
		using timepoint = typename CLOCK::time_point;
		const timepoint start_ = CLOCK::now();

		double nano() const
		{
			std::atomic_thread_fence(std::memory_order_relaxed);
			double rez_ = static_cast<double>((CLOCK::now() - start_).count());
			std::atomic_thread_fence(std::memory_order_relaxed);
			return rez_;
		}

		double micro() const { return nano() / 1000.0; }
		double milli() const { return micro() / 1000.0; }
		double seconds() const { return milli() / 1000.0; }
		// double decimal3(double arg) { return (std::round(arg * 1000)) / 1000; }

		enum class kind
		{
			nano,
			micro,
			milli,
			second
		};

		
		   /// timer timer_{} ;
		   /// auto microsecs_ = as_buffer( timer_ );
		   /// auto secs_ = as_buffer( timer_, timer::kind::second );
		
		friend buffer as_buffer(timer const& timer_, kind which_ = kind::milli)
		{
			buffer retval{ char{0} };
			double arg{};
			char const* unit_{};
			switch (which_)
			{
			case kind::nano:
				arg = timer_.nano();
				unit_ = " nano seconds ";
				break;
			case kind::micro:
				arg = timer_.micro();
				unit_ = " micro seconds ";
				break;
			case kind::milli:
				arg = timer_.milli();
				unit_ = " milli seconds ";
				break;
			default: //seconds
				arg = timer_.seconds();
				unit_ = " seconds ";
			}
			std::snprintf(retval.data(), retval.size(), "%.3f%s", arg, unit_);
			return retval;
		}
	}; // timer

} // lf_queue::test