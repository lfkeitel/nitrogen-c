#ifndef nmempool
#define nmemppol

void allocate_pool(void);
void* nmalloc(void);
void nfree(void* p);
void deallocate_pool(void);

#endif