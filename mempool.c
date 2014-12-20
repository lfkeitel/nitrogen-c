/*
 *	Memory management pool for NVAL types.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <valgrind/memcheck.h>
#include "ncore.h"

#define POOL_SIZE 12000

typedef struct mem_control_block {
	bool is_used;
} mem_control_block;

static bool ready;
static void* memory_pool_start;
static void* memory_pool_end;
static size_t mem_chunk_size = sizeof(nval) + sizeof(mem_control_block);

void allocate_pool(void) {
	memory_pool_start = malloc(POOL_SIZE * mem_chunk_size);
	memory_pool_end = memory_pool_start + (POOL_SIZE * mem_chunk_size) - mem_chunk_size; // Last usable address
	VALGRIND_CREATE_MEMPOOL(memory_pool_start, 0, 0);
	ready = true;
	return;
}

void* nmalloc(void) {
	/* If the pool hasn't been allocated, allocate it */
	if (!ready) {
		allocate_pool();
	}

	/* Prepare variables */
	void* current_location = memory_pool_start;
	mem_control_block* current_location_mcb;
	void* memory_location = memory_pool_start;

	while (current_location < memory_pool_end) {
		current_location_mcb = current_location;
		if (current_location_mcb == NULL || current_location_mcb->is_used) {
			current_location = current_location + mem_chunk_size;
			continue;
		} else {
			current_location_mcb->is_used = true;
			memory_location = current_location;
			break;
		}
	}

	if (!memory_location) {
		printf("%s\n", "You're screwed");
	}
	memory_location = memory_location + sizeof(mem_control_block);
	VALGRIND_MEMPOOL_ALLOC(memory_pool_start, memory_location, sizeof(nval));
	return memory_location;
}

void nfree(void* p) {
	mem_control_block* mcb;
	mcb = p - sizeof(mem_control_block);
	mcb->is_used = false;
	VALGRIND_MEMPOOL_FREE(memory_pool_start, p);
	return;
}

void deallocate_pool(void) {
	VALGRIND_DESTROY_MEMPOOL(memory_pool_start);
	free(memory_pool_start);
	return;
}