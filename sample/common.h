#pragma once
/*
common to all samples
*/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define STRICT 1
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h> // CRITICAL_SECTION

#ifdef __cplusplus
namespace common
{
extern "C"
{
#endif

    /// --------------------------------------------------------------------------------------------
    /// we need to make coomon function work in presence of multiple threads
    typedef struct
    {
        bool initalized;
        CRITICAL_SECTION crit_sect;
    } synchro_type;

    void exit_common(void);

    inline synchro_type *common_initor()
    {
        static synchro_type synchro_ = {false};
        if (!synchro_.initalized)
        {
            InitializeCriticalSection(&synchro_.crit_sect);
            synchro_.initalized = true;
            atexit(exit_common);
        }

        return &synchro_;
    }

    void exit_common(void)
    {
        synchro_type crit_ = *common_initor();

        if (crit_.initalized)
        {
            DeleteCriticalSection(&crit_.crit_sect);
            crit_.initalized = false;
        }
    }

    void synchro_enter() { EnterCriticalSection(&common_initor()->crit_sect); }
    void synchro_leave() { LeaveCriticalSection(&common_initor()->crit_sect); }

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
    /// so for clear confusion free thread to thread messaging use heap

    /// we always `name` the threads by sending them this through
    /// _beginthreadex() call, as the fourth argument
    inline char *make_name_(unsigned id_, origin origin_)
    {
        synchro_enter();
        assert(origin_.val);
        char *retval = (char *)calloc(0xF, sizeof(char));
        assert(retval);
        int count = snprintf(retval, 0xF, "%s: %3d", origin_.val, id_);
        assert(!(count < 0));
        synchro_leave();
        return retval;
    }
    inline void free_name_(char *pn_)
    {
        synchro_enter();
        free(pn_);
        pn_ = 0;
        synchro_leave();
    }
    /// we can not tell if vp_ is coming from a heap
    /// or from a stack
    inline char *print_name_(void *vp_, origin origin_)
    {
        synchro_enter();
        assert(vp_);
        assert(origin_.val);
        char *pn_ = (char *)vp_;
        printf("\n%16s [%16s] has started", origin_.val, pn_);
        fflush(0);
        synchro_leave();
        return pn_;
    }
    inline char *make_message_(char *producer_name_)
    {
        synchro_enter();
        assert(producer_name_);
        char *retval = (char *)calloc(0xFF, sizeof(char));
        assert(retval);
        int count = snprintf(retval, 0xFF, "Message from: %s", producer_name_);
        assert(!(count < 0));
        synchro_leave();
        return retval;
    }
    inline void free_message_(char *pn_)
    {
        synchro_enter();
        free(pn_);
        pn_ = 0;
        synchro_leave();
    }

#ifdef __cplusplus
} // extern "C"
} // common
#endif
