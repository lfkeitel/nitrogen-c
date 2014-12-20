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
#define POOL_SIZE 1000

/* Group of memory pools for nvals */
static memory_pool* nval_mem_pool[MAX_NUM_OF_POOLS];

/* Statistics */
static int total_allocated_chunks = 0;
static int total_currently_allocated_chunks = 0;
static int highest_allocated_chunks = 0;

static int created_pools = 0;
static size_t nval_chunk_size = sizeof(nval) + sizeof(mem_control_block);

/* Create new pools at index pnum */
void create_pool(int pnum) {
	nval_mem_pool[pnum] = malloc(sizeof(memory_pool));
	nval_mem_pool[pnum]->mem_chunk_size = nval_chunk_size;
	nval_mem_pool[pnum]->memory_pool_start =  malloc(POOL_SIZE * nval_chunk_size);
	nval_mem_pool[pnum]->memory_pool_last_assignable = nval_mem_pool[pnum]->memory_pool_start + (POOL_SIZE * nval_mem_pool[pnum]->mem_chunk_size) - nval_mem_pool[pnum]->mem_chunk_size; // Last usable address
	nval_mem_pool[pnum]->memory_pool_end = nval_mem_pool[pnum]->memory_pool_start + (POOL_SIZE * nval_mem_pool[pnum]->mem_chunk_size);
	nval_mem_pool[pnum]->chunks_allocated = 0;
	VALGRIND_CREATE_MEMPOOL(nval_mem_pool[pnum], 0, 0);
	created_pools++;
	return;
}

/* Find and return a pointer to an nval sized chunk */
void* nmalloc(void) {
	void* current_location;
	mem_control_block* current_location_mcb;
	void* memory_location = NULL;

	/* Loop through pools to find a free address */
	for (int i = 0; i < created_pools; i++) {
		/* If a pool is uninitialized, all pool after it will be too */
		if (nval_mem_pool[i] == NULL) {
			break;
		}

		current_location = nval_mem_pool[i]->memory_pool_start;
		memory_location = nval_mem_pool[i]->memory_pool_start;

		/* Loop over pool address range to find a free memory chunk */
		while (current_location < nval_mem_pool[i]->memory_pool_last_assignable) {
			current_location_mcb = current_location;
			if (current_location_mcb->is_used) {
				/* Chunk in use */
				current_location = current_location + nval_mem_pool[i]->mem_chunk_size;
				continue;
			} else {
				/* Chunk is not in use or is unitialized */
				current_location_mcb->is_used = true;
				memory_location = current_location + sizeof(mem_control_block);
				nval_mem_pool[i]->chunks_allocated++;
				/* Stats */
				total_allocated_chunks++;
				total_currently_allocated_chunks++;
				if (total_currently_allocated_chunks > highest_allocated_chunks) {
					highest_allocated_chunks = total_allocated_chunks;
				}
				VALGRIND_MEMPOOL_ALLOC(nval_mem_pool[i], memory_location, sizeof(nval));
				return memory_location;
				break;
			}
		}
	}

	/* All pools have been checked and are exhausted */
	if (created_pools < MAX_NUM_OF_POOLS) {
		create_pool(created_pools);
		return nmalloc();
	} else {
		printf("No more memory available\n");
	}

	return NULL;
}

void nfree(void* p) {
	mem_control_block* mcb;
	mcb = p - sizeof(mem_control_block);
	mcb->is_used = false;

	for (int i = 0; i < created_pools; i++) {
		if (p >= nval_mem_pool[i]->memory_pool_start && p < nval_mem_pool[i]->memory_pool_end) {
			nval_mem_pool[i]->chunks_allocated--;
			VALGRIND_MEMPOOL_FREE(nval_mem_pool[i], p);
		}
	}
	total_currently_allocated_chunks--;
	return;
}

void deallocate_pools(void) {
	for (int i = 0; i < created_pools; i++) {
		free(nval_mem_pool[i]->memory_pool_start);
		free(nval_mem_pool[i]);
		VALGRIND_DESTROY_MEMPOOL(nval_mem_pool[i]);
	}
	return;
}

void pool_stats(void) {
	printf("Number of Pools: %d\n", created_pools);
	printf("Size of mem_control_block: %li\n", sizeof(mem_control_block));
	printf("Size of Pool Header: %li\n", sizeof(memory_pool));
	printf("Currently Allocated Chunks: %d\n", total_currently_allocated_chunks);
	printf("Highest Allocated Chunks: %d\n", highest_allocated_chunks);
	printf("Total Allocated Chunks: %d\n", total_allocated_chunks);

	for (int i = 0; i < created_pools; i++) {
		putchar('\n');
		printf("  Stats for Pool %d\n", i);
		printf("    Chunk Allocated: %d\n", nval_mem_pool[i]->chunks_allocated);
		printf("    Header Address: %p\n", nval_mem_pool[i]);
		printf("    Start Address: %p\n", nval_mem_pool[i]->memory_pool_start);
		printf("    End Address: %p\n", nval_mem_pool[i]->memory_pool_end);
		printf("    Last Assignable Address: %p\n", nval_mem_pool[i]->memory_pool_last_assignable);
		printf("    Chunk Size: %li\n", nval_mem_pool[i]->mem_chunk_size);
	}
	putchar('\n');
	return;
}
