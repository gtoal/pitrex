/**
 * @file malloc.c
 *
 */
/* This code is inspired by:
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
 * https://github.com/rsta2/circle/blob/master/lib/alloc.cpp
 */
/* Copyright (C) 2017-2018 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#ifdef MEM_DEBUG
#include <stdio.h>
#endif

extern unsigned char heap_low; /* Defined by the linker */
extern unsigned char heap_top; /* Defined by the linker */

#define BLOCK_MAGIC	0x424C4D43

static unsigned char *next_block = &heap_low;
static unsigned char *block_limit = &heap_top;

struct block_header {
	unsigned int magic;
	unsigned int size ;
	struct block_header *next;
	unsigned char data[0];
} PACKED;

struct block_bucket {
	unsigned int size;
#ifdef MEM_DEBUG
	unsigned int count;
	unsigned int max_count;
#endif
	struct block_header *free_list;
};

static struct block_bucket s_block_bucket[] __attribute__((aligned(4))) = {{0x40}, {0x400}, {0x1000}, {0x4000}, {0x40000}, {0x80000}, {0}};

size_t get_allocated(void *p) {
	if (p == 0) {
		return 0;
	}

	struct block_header *pBlockHeader = (struct block_header *) ((void *) p - sizeof(struct block_header));

	assert(pBlockHeader->magic == BLOCK_MAGIC);
	if (pBlockHeader->magic != BLOCK_MAGIC) {
		return 0;
	}

	return pBlockHeader->size;
}

void *malloc(size_t size) {
	struct block_bucket *bucket;
	struct block_header *header;

	if (size == 0) {
		return NULL;
	}

	for (bucket = s_block_bucket; bucket->size > 0; bucket++) {
		if (size <= bucket->size) {
			size = bucket->size;
#ifdef MEM_DEBUG
			if (++bucket->count > bucket->max_count) {
				bucket->max_count = bucket->count;
			}
#endif
			break;
		}
	}

	if (bucket->size > 0 && (header = bucket->free_list) != 0) {
		assert(header->magic == BLOCK_MAGIC);
		bucket->free_list = header->next;
	} else {
		header = (struct block_header *) next_block;

		const size_t t1 = sizeof(struct block_header) + size;
		const size_t t2 = (t1 + (size_t) 15) & ~(size_t) 15;

		unsigned char *next = next_block + t2;

		assert(((unsigned)header & (unsigned)3) == 0);
		assert(((unsigned)next & (unsigned)3) == 0);

		if (next > block_limit) {
			return NULL;
		} else {
			next_block = next;
		}

		header->magic = BLOCK_MAGIC;
		header->size = (unsigned) size;
	}

	header->next = 0;
#ifdef MEM_DEBUG
	printf("malloc: pBlockHeader = %p, size = %d\n", header, (int) size);
#endif

	assert(((unsigned)header->data & (unsigned)3) == 0);
	return (void *)header->data;
}

void free(void *p) {
	struct block_bucket *bucket;

	if (p == 0) {
		return;
	}

	struct block_header *header = (struct block_header *) ((void *) p - sizeof(struct block_header));

#ifdef MEM_DEBUG
	printf("free: pBlockHeader = %p, pBlock = %p\n", header, p);
#endif

	assert(header->magic == BLOCK_MAGIC);
	if (header->magic != BLOCK_MAGIC) {
		return;
	}

	for (bucket = s_block_bucket; bucket->size > 0; bucket++) {
		if (header->size == bucket->size) {

			header->next = bucket->free_list;
			bucket->free_list = header;
#ifdef MEM_DEBUG
			bucket->count--;
#endif
			break;
		}
	}
}

void *calloc(size_t n, size_t size) {
	size_t total;
	void *p;

	if ((n == 0) || (size == 0)) {
		return NULL;
	}

	total = n * size;

	p = malloc(total);

	if (p == NULL) {
		return NULL;
	}

	assert(((unsigned)p & (unsigned)3) == 0);

	uint32_t *dst32 = (uint32_t *) p;

	while (total >= 4) {
		*dst32++ = (uint32_t) 0;
		total -= 4;
	}

	uint8_t *dst8 = (uint8_t *) dst32;

	while (total--) {
		*dst8++ = (uint8_t) 0;
	}

	assert(((void *)dst8 - (void *)p) == (n * size));

	return (void *) p;
}

void *realloc(void *ptr, size_t size) {
	size_t current_size;

	if (ptr == 0) {
		void *newblk = malloc(size);
		return newblk;
	}

	if (size == 0) {
		free(ptr);
		return NULL;
	}

	current_size = get_allocated(ptr);

	if (current_size >= size) {
		return ptr;
	}

	void *newblk = malloc(size);

	if (newblk != NULL) {
		assert(((unsigned )newblk & (unsigned )3) == 0);
		assert(((unsigned )ptr & (unsigned )3) == 0);

		const uint32_t *src32 = (const uint32_t *) ptr;
		uint32_t *dst32 = (uint32_t *) newblk;

		size_t count = size;

		while (count >= 4) {
			*dst32++ = *src32++;
			count -= 4;
		}

		const uint8_t *src8 = (const uint8_t *) src32;
		uint8_t *dst8 = (uint8_t *) dst32;

		while (count--) {
			*dst8++ = *src8++;
		}

		assert(((void *)dst8 - (void *)newblk) == size);

		free(ptr);
	}

	return newblk;
}

void mem_info(void) {
#ifdef MEM_DEBUG
	struct block_bucket *pBucket;
	struct block_header *pBlockHeader;
	printf("s_pNextBlock = %p\n", next_block);

	for (pBucket = s_block_bucket; pBucket->size > 0; pBucket++) {
		printf("malloc(%d): %d blocks (max %d), FreeList %p\n", (unsigned) pBucket->size, (unsigned) pBucket->count, (unsigned) pBucket->max_count, pBucket->free_list);
		if ((pBlockHeader = pBucket->free_list) != 0) {
			while (1==1) {
				printf("\t %p:%p size %d (%p)\n", pBlockHeader, pBlockHeader->data, pBlockHeader->size, pBlockHeader->next);
				if (pBlockHeader->next == 0) {
					break;
				}
				pBlockHeader = pBlockHeader->next;
			}
		}
	}
#endif
}
