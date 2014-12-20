#ifndef nmempool
#define nmemppol

typedef struct mem_control_block {
    bool is_used;
} mem_control_block;

typedef struct memory_pool {
    int chunks_allocated;
    void* memory_pool_start;
    void* memory_pool_end;
    void* memory_pool_last_assignable;
    size_t mem_chunk_size;
} memory_pool;

void create_pool(int pnum);
void* nmalloc(void);
void nfree(void* p);
void deallocate_pools(void);
void pool_stats(void);

#endif