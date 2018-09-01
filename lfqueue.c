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
#define __LFQ_LOAD_FENCE() __asm volatile( "lfence" ::: "memory")
#define __LFQ_STORE_FENCE() __asm volatile( "sfence" ::: "memory" )

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
#define __LFQ_LOAD_FENCE LoadFence
#define __LFQ_STORE_FENCE StoreFence

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
#define __LFQ_LOAD_FENCE() asm lfence
#define __LFQ_STORE_FENCE() asm sfence

#endif
#include <windows.h>
#define __LFQ_YIELD_THREAD SwitchToThread
#endif

#include "lfqueue.h"
#define DEF_LFQ_ASSIGNED_SPIN 2048

static void *dequeue_(lfqueue_t *lfqueue);
static int enqueue_(lfqueue_t *lfqueue, void* value);

static void *
dequeue_(lfqueue_t *lfqueue) {
	lfqueue_cas_node_t *head, *next;
	void *val;
	// int ispin = DEF_LFQ_ASSIGNED_SPIN;

	for (;;) {
		head = lfqueue->head;
		if (__LFQ_BOOL_COMPARE_AND_SWAP(&lfqueue->head, head, head->next)) {
			val = head->value;
			if (__LFQ_BOOL_COMPARE_AND_SWAP(&head->value, val, NULL)) {
				return val;
			}
		}
	}
	return NULL;
}

static int
enqueue_(lfqueue_t *lfqueue, void* value) {
	lfqueue_cas_node_t *tail, *node, *next;
	for (;;) {
		tail = lfqueue->tail;
		__LFQ_SYNC_MEMORY();
		if (tail->next != lfqueue->head) {
			next = tail->next;
			if (__LFQ_BOOL_COMPARE_AND_SWAP(&lfqueue->tail, tail, next)) {
				if (!__LFQ_BOOL_COMPARE_AND_SWAP(&tail->value, NULL, value)) {
					// Value over write? expand the size
					continue;
				}
				return 0;
			}
		}
	}
	return -1;
}

int
lfqueue_init(lfqueue_t *lfqueue, size_t queue_size, int num_concurrent, int expandable/*expandable queue sz * 2 */) {
	int i;

	if(queue_size < (num_concurrent * num_concurrent)) {
		perror("At least 1024 queue size to avoid infinite loop");
		return -1;
	}

	lfqueue_cas_node_t *base = malloc(queue_size * sizeof(lfqueue_cas_node_t));
	if (base == NULL) {
		return errno;
	}
	lfqueue->base = lfqueue->head = lfqueue->tail = base;
	lfqueue->size = 0;
	lfqueue->capacity = queue_size;
	lfqueue->expandable = expandable;
	lfqueue->expandable_sz = queue_size;
	
	for (i = 0; i < queue_size - 1; i++) {
		base[i].value = NULL;
		base[i].next = base + i + 1;
	}
	base[i].value = NULL;
	base[i].next = base;

	return 0;
}

void
lfqueue_destroy(lfqueue_t *lfqueue) {
	void* p;
	while ((p = lfqueue_deq(lfqueue))) {
		free(p);
	}
	free(lfqueue->base);
	lfqueue->size = 0;
}

int
lfqueue_enq(lfqueue_t *lfqueue, void *value) {
	__LFQ_SYNC_MEMORY();
	if (lfqueue->size >= lfqueue->capacity) {
		// TODO if(lfqueue->expandable)
		return -1;
	}

	if (enqueue_(lfqueue, value)) {
		return -1;
	}
	__LFQ_ADD_AND_FETCH(&lfqueue->size, 1);
	return 0;
}

void*
lfqueue_deq(lfqueue_t *lfqueue) {
	void *v;
	if (/*lfqueue->size &&*/
	    (v = dequeue_(lfqueue))
	) {
		__LFQ_FETCH_AND_ADD(&lfqueue->size, -1);
		return v;
	}
	return NULL;
}

size_t
lfqueue_size(lfqueue_t *lfqueue) {
	return __LFQ_ADD_AND_FETCH(&lfqueue->size, 0);
}

