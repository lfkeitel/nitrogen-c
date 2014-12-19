/*
 *	Memory management pool for NVAL types.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "ncore.h"

#define POOL_SIZE 1000

typedef struct mem_control_block {
	bool is_used;
} mem_control_block;

static bool ready;
static void* memory_pool_start;
static void* memory_pool_end;
static size_t mem_chunk = sizeof(nval) + sizeof(mem_control_block);

void allocate_pool(void) {
	memory_pool_start = malloc(POOL_SIZE * mem_chunk);
	memory_pool_start = memory_pool_start+1;
	memory_pool_end = memory_pool_start + (POOL_SIZE * mem_chunk);
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
	void* memory_location = 0;

	while (current_location < memory_pool_end) {
		current_location_mcb = current_location;
		if (current_location_mcb->is_used) {
			current_location = current_location + mem_chunk;
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
	return memory_location;
}

void nfree(void* p) {
	mem_control_block* mcb;
	mcb = p - sizeof(mem_control_block);
	mcb->is_used = false;
	return;
}

void deallocate_pool(void) {
	free(memory_pool_start);
	return;
}