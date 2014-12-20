/*
 *	Memory management pool for NVAL types.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <valgrind/memcheck.h>

#include "ncore.h"
#include "mempool.h"

#define MAX_NUM_OF_POOLS 10
#define POOL_SIZE 12000

static memory_pool* nval_mem_pool[MAX_NUM_OF_POOLS];

static bool ready;
static size_t nval_chunk_size = sizeof(nval) + sizeof(mem_control_block);

/* Allocate first pool */
void allocate_pool(void) {
	create_pool(0);
	ready = true;
	return;
}

/* Create new pools at index pnum */
void create_pool(int pnum) {
	nval_mem_pool[pnum] = malloc(sizeof(memory_pool));
	nval_mem_pool[pnum]->mem_chunk_size = nval_chunk_size;
	nval_mem_pool[pnum]->memory_pool_start = malloc(POOL_SIZE * nval_mem_pool[pnum]->mem_chunk_size);
	nval_mem_pool[pnum]->memory_pool_last_assignable = nval_mem_pool[pnum]->memory_pool_start + (POOL_SIZE * nval_mem_pool[pnum]->mem_chunk_size) - nval_mem_pool[pnum]->mem_chunk_size; // Last usable address
	nval_mem_pool[pnum]->memory_pool_end = nval_mem_pool[pnum]->memory_pool_start + (POOL_SIZE * nval_mem_pool[pnum]->mem_chunk_size);
	nval_mem_pool[pnum]->chunks_allocated = 0;
	VALGRIND_CREATE_MEMPOOL(nval_mem_pool[pnum]->memory_pool_start, 0, 0);
	return;
}

/* Find and return a pointer to an nval sized chunk */
void* nmalloc(void) {
	/* If the pool hasn't been allocated, allocate it */
	if (!ready) {
		allocate_pool();
	}

	void* current_location;
	mem_control_block* current_location_mcb;
	void* memory_location;

	/* Loop through pools to find a free address */
	for (int i = 0; i < MAX_NUM_OF_POOLS; i++) {
		/* If a pool is uninitialized, all pool after it will be too */
		if (nval_mem_pool[i] == NULL) {
			break;
		}

		current_location = nval_mem_pool[0]->memory_pool_start;
		memory_location = nval_mem_pool[0]->memory_pool_start;

		/* Loop over pool address range to find a free memory chunk */
		while (current_location < nval_mem_pool[0]->memory_pool_end) {
			current_location_mcb = current_location;
			if (current_location_mcb->is_used) {
				/* Chunk in use */
				current_location = current_location + nval_mem_pool[0]->mem_chunk_size;
				continue;
			} else {
				/* Chunk is not in use or is unitialized */
				current_location_mcb->is_used = true;
				memory_location = current_location;
				break;
			}
		}

		if (memory_location) {
			/* Memory location has been found */
			break;
		}
	}

	if (!memory_location) {
		/* All pools have been checked and are exhausted */
		printf("%s\n", "You're screwed");
	}
	memory_location = memory_location + sizeof(mem_control_block);
	VALGRIND_MEMPOOL_ALLOC(nval_mem_pool[0]->memory_pool_start, memory_location, sizeof(nval));
	nval_mem_pool[0]->chunks_allocated++;
	return memory_location;
}

void nfree(void* p) {
	mem_control_block* mcb;
	mcb = p - sizeof(mem_control_block);
	mcb->is_used = false;
	VALGRIND_MEMPOOL_FREE(nval_mem_pool[0]->memory_pool_start, p);
	nval_mem_pool[0]->chunks_allocated--;
	return;
}

void deallocate_pool(void) {
	VALGRIND_DESTROY_MEMPOOL(nval_mem_pool[0]->memory_pool_start);
	free(nval_mem_pool[0]->memory_pool_start);
	return;
}

int pool_stats(void) {
	return nval_mem_pool[0]->chunks_allocated;
}
