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

    /// Common_Origin is a "strong type" (c) 2019 dbj@dbj.org
    ///
    /// void fun(Common_Origin);
    ///
    /// is infinitely better than
    ///
    /// void fun ( char * );
    ///
    /// calling is even better
    ///
    /// fun((Common_Origin){"Producer"});
    ///
    /// almost like named argument, but not, because
    /// the name required is a type
    /// thus type and value cleraly stated in a call
    /// better than C++ variant:
    ///
    /// C++
    /// fun({"Producer"}); // what is the type passed?
    ///
    typedef struct
    {
        const char *val;
    } Common_Origin;

#define Common_Name_Length 0xFF
    typedef struct
    {
        char val[Common_Name_Length];
    } Common_Name;

    typedef struct
    {
        unsigned val;
    } Common_Id;

    /// --------------------------------------------------------------------------------------------
    /// one can pass objects on stack to threads, but stack is shared by default between threads
    /// so for clear confusion free thread to thread messaging use heap

    /// we always `name` the threads by sending them this through
    /// _beginthreadex() call, as the fourth argument
    inline Common_Name make_name_(Common_Id id_, Common_Origin origin_)
    {
        synchro_enter();
        assert(origin_.val);
        Common_Name retval = {{0}};
        int count = snprintf(retval.val, Common_Name_Length, "%s: %3d", origin_.val, id_.val);
        assert(!(count < 0));
        synchro_leave();
        return retval;
    }
    inline void free_name_(Common_Name name_)
    {
        synchro_enter();
        memset(name_.val, 0, Common_Name_Length);
        synchro_leave();
    }
    /// we can not tell if vp_ is coming from a heap
    /// or from a stack
//     inline char *print_name_(void *vp_, Common_Origin origin_)
//     {
//         synchro_enter();
//         assert(vp_);
//         assert(origin_.val);
//         char *pn_ = (char *)vp_;
// #ifdef COMMON_TRACING
//         printf("\n%16s [%16s] has started", origin_.val, pn_);
//         printf(" in thread [%6d]", (int)GetCurrentThreadId());
//         fflush(0);
// #endif
//         synchro_leave();
//         return pn_;
//     }
    /// -----------------------------------------------------------------------------
#define Common_Message_Length 0xFF
    typedef struct
    {
        char val[Common_Message_Length];
    } Common_Message;

    /// make message on the heap
    /// for sane inter threading
    inline Common_Message *make_message_(Common_Origin origin_)
    {
        synchro_enter();
        assert(origin_.val);
        Common_Message *retval =
            (Common_Message *)calloc(1, sizeof(Common_Message));
        assert(retval);
        int count = snprintf(retval->val, Common_Message_Length, "Message from: %s", origin_.val);
        assert(!(count < 0));
        synchro_leave();
        return retval;
    }
    inline void free_message_(Common_Message *msg_)
    {
        synchro_enter();
        free(msg_);
        synchro_leave();
    }

#ifdef __cplusplus
} // extern "C"
} // common
#endif
