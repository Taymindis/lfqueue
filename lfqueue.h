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

#ifndef LFQUEUE_H
#define LFQUEUE_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct lfqueue_cas_node_s {
	void * value;
	struct lfqueue_cas_node_s *next, *nextfree;
	volatile int retain, active;
} lfqueue_cas_node_t;

typedef struct {
	lfqueue_cas_node_t *head, *tail, *root_free, *move_free;
	volatile size_t size;
	int freeing;
	unsigned int _ndeq_in_sec;
	volatile unsigned int freecount;
} lfqueue_t;

/** Dequeue might be failed if the Deq thread access to the queue more than num_concurrent_consume **/
int   lfqueue_init(lfqueue_t *lfqueue, unsigned int n_deq_in_sec);
int   lfqueue_enq(lfqueue_t *lfqueue, void *value);
void *lfqueue_deq(lfqueue_t *lfqueue);
void lfqueue_destroy(lfqueue_t *lfqueue);
size_t lfqueue_size(lfqueue_t *lfqueue);
void lfqueue_usleep(unsigned int usec);
#ifdef __cplusplus
}
#endif
#endif

