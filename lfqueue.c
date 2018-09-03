/*
*
* BSD 2-Clause License
*
* Copyright (c) 2018, Taymindis Woon
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__

#include <pthread.h>
#include <sched.h>

#define __LFQ_VAL_COMPARE_AND_SWAP __sync_val_compare_and_swap
#define __LFQ_BOOL_COMPARE_AND_SWAP __sync_bool_compare_and_swap
#define __LFQ_FETCH_AND_ADD __sync_fetch_and_add
#define __LFQ_ADD_AND_FETCH __sync_add_and_fetch
#define __LFQ_YIELD_THREAD sched_yield
#define __LFQ_SYNC_MEMORY __sync_synchronize
#define __LFQ_LOAD_MEMORY() __asm volatile( "lfence" )
#define __LFQ_STORE_MEMORY() __asm volatile( "sfence" )

#else

#include <Windows.h>
#ifdef _WIN64
inline BOOL __SYNC_BOOL_CAS(LONG64 volatile *dest, LONG64 input, LONG64 comparand) {
	return InterlockedCompareExchangeNoFence64(dest, input, comparand) == comparand;
}
#define __LFQ_VAL_COMPARE_AND_SWAP(dest, comparand, input) \
    InterlockedCompareExchangeNoFence64((LONG64 volatile *)dest, (LONG64)input, (LONG64)comparand)
#define __LFQ_BOOL_COMPARE_AND_SWAP(dest, comparand, input) \
    __SYNC_BOOL_CAS((LONG64 volatile *)dest, (LONG64)input, (LONG64)comparand)
#define __LFQ_FETCH_AND_ADD InterlockedExchangeAddNoFence64
#define __LFQ_ADD_AND_FETCH InterlockedAddNoFence64
#define __LFQ_SYNC_MEMORY MemoryBarrier

#else
#ifndef asm
#define asm __asm
#endif
inline BOOL __SYNC_BOOL_CAS(LONG volatile *dest, LONG input, LONG comparand) {
	return InterlockedCompareExchangeNoFence(dest, input, comparand) == comparand;
}
#define __LFQ_VAL_COMPARE_AND_SWAP(dest, comparand, input) \
    InterlockedCompareExchangeNoFence((LONG volatile *)dest, (LONG)input, (LONG)comparand)
#define __LFQ_BOOL_COMPARE_AND_SWAP(dest, comparand, input) \
    __SYNC_BOOL_CAS((LONG volatile *)dest, (LONG)input, (LONG)comparand)
#define __LFQ_FETCH_AND_ADD InterlockedExchangeAddNoFence
#define __LFQ_ADD_AND_FETCH InterlockedAddNoFence
#define __LFQ_SYNC_MEMORY() asm mfence

#endif
#include <windows.h>
#define __LFQ_YIELD_THREAD SwitchToThread
#endif

#include "lfqueue.h"
#define DEF_LFQ_ASSIGNED_SPIN 2048

//static lfqueue_cas_node_t* __lfq_assigned(lfqueue_t *);
static void __lfq_recycle_free(lfqueue_t *, lfqueue_cas_node_t*);

static void *
dequeue_(lfqueue_t *lfqueue) {
	lfqueue_cas_node_t *head, *next;
	void *val;
	int goreturn = 0;
	for (;;) {
		head = lfqueue->head;
		__LFQ_FETCH_AND_ADD(&head->retain, 1);
		if (__LFQ_BOOL_COMPARE_AND_SWAP(&lfqueue->head, head, head)) {
			next = head->next;
			if (__LFQ_BOOL_COMPARE_AND_SWAP(&lfqueue->tail, head, head)) {
				if (next == NULL) {
					val = NULL;
					goreturn = 1;
				}
			} else {
				if (next) {
					val = next->value;
					if (__LFQ_BOOL_COMPARE_AND_SWAP(&lfqueue->head, head, next)) {
						head->active = 0;
						goreturn = 1;
					}
				} else {
					val = NULL;
					goreturn = 1;
				}
			}
		}
		__LFQ_FETCH_AND_ADD(&head->retain, -1);
		// __asm volatile("" ::: "memory");
		__LFQ_SYNC_MEMORY();
		// __asm volatile("" ::: "memory");
		__lfq_recycle_free(lfqueue, head);

		if (goreturn)
			return val;
	}

	return val;
}

static int
enqueue_(lfqueue_t *lfqueue, void* value) {
	lfqueue_cas_node_t *tail, *node;
	node = (lfqueue_cas_node_t*) malloc(sizeof(lfqueue_cas_node_t));
	if (node == NULL) {
		perror("malloc");
		return errno;
	}
	node->value = value;
	node->next = NULL;
	node->active = 1;
	node->retain = 0;
	node->nextfree = NULL;
	// int status;
	for (;;) {
		__LFQ_SYNC_MEMORY();
		tail = lfqueue->tail;
		if (__LFQ_BOOL_COMPARE_AND_SWAP(&tail->next, NULL, node)) {
			// compulsory swap as tail->next is no NULL anymore, it has fenced on other thread
			__LFQ_BOOL_COMPARE_AND_SWAP(&lfqueue->tail, tail, node);
			return 0;
		}
	}

	/*It never be here*/
	return -1;
}

int
lfqueue_init(lfqueue_t *lfqueue, unsigned int n_deq_in_sec) {
	/** At least 1024 dequeue free level **/
	if (n_deq_in_sec < 1024) {
		n_deq_in_sec = 1024;
	}

	lfqueue_cas_node_t *base = malloc(sizeof(lfqueue_cas_node_t));
	lfqueue_cas_node_t *freebase = malloc(sizeof(lfqueue_cas_node_t));
	if (base == NULL || freebase == NULL) {
		perror("malloc");
		return errno;
	}
	base->value = NULL;
	base->active = 1;
	base->retain = 0;
	base->next = NULL;
	base->nextfree = NULL;

	freebase->value = NULL;
	freebase->active = 0;
	freebase->retain = 0;
	freebase->next = NULL;
	freebase->nextfree = NULL;

	lfqueue->head = lfqueue->tail = base; // Not yet to be free for first node only
	lfqueue->root_free = lfqueue->move_free = freebase; // Not yet to be free for first node only
	lfqueue->size = 0;
	lfqueue->freecount = 0;
	lfqueue->_ndeq_in_sec = n_deq_in_sec;

	return 0;
}

void
lfqueue_destroy(lfqueue_t *lfqueue) {
	void* p;
	while ((p = lfqueue_deq(lfqueue))) {
		free(p);
	}
	// Clear the recycle chain nodes
	lfqueue_cas_node_t *rtfree = lfqueue->root_free, *nextfree;
	while (rtfree != lfqueue->move_free) {
		nextfree = rtfree->nextfree;
		free(rtfree);
		rtfree = nextfree;
	}
	free(rtfree);

	lfqueue->size = 0;
}

int
lfqueue_enq(lfqueue_t *lfqueue, void *value) {
	if (enqueue_(lfqueue, value)) {
		return -1;
	}
	__LFQ_ADD_AND_FETCH(&lfqueue->size, 1);
	return 0;
}

void*
lfqueue_deq(lfqueue_t *lfqueue) {
	void *v;
	if (//__LFQ_ADD_AND_FETCH(&lfqueue->size, 0) &&
	    (v = dequeue_(lfqueue))
	) {

		__LFQ_FETCH_AND_ADD(&lfqueue->size, -1);
		return v;
	}
	// Rest the thread for other thread, to avoid keep looping force
	lfqueue_usleep(1000);
	return NULL;
}

size_t
lfqueue_size(lfqueue_t *lfqueue) {
	return __LFQ_ADD_AND_FETCH(&lfqueue->size, 0);
}



static void __lfq_recycle_free(lfqueue_t *q, lfqueue_cas_node_t* freenode) {
	if (__LFQ_BOOL_COMPARE_AND_SWAP(&freenode->retain, 0, 0)  &&
	        __LFQ_BOOL_COMPARE_AND_SWAP(&freenode->active, 0, -1)  ) {
		lfqueue_cas_node_t *freed;
		do {
			freed = q->move_free;
		} while (!__LFQ_BOOL_COMPARE_AND_SWAP(&freed->nextfree, NULL, freenode) );

		__LFQ_BOOL_COMPARE_AND_SWAP(&q->move_free, freed, freenode);

		if ( __LFQ_FETCH_AND_ADD(&q->freecount, 1) > q->_ndeq_in_sec) {
			__LFQ_FETCH_AND_ADD(&q->freecount, -1); // keep in _ndep_in_sec range
			do {
				freed = q->root_free;
			} while (!__LFQ_BOOL_COMPARE_AND_SWAP(&q->root_free, freed, freed->nextfree) );
			// printf("FREE???? %p, freecount %d\n", freed, q->freecount);
			free(freed);
		}
	}
}

void lfqueue_usleep(unsigned int usec) {
#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	usleep(usec);
#pragma GCC diagnostic pop
#else
	HANDLE hTimer;
	LARGE_INTEGER DueTime;
	DueTime.QuadPart = -(10 * (__int64)usec);
	hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(hTimer, &DueTime, 0, NULL, NULL, 0);
	if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0) {
		/* TODO do nothing*/
	}
#endif
}

#ifdef __cplusplus
}
#endif
