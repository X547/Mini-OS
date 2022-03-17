#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stddef.h>

extern "C" {

void *malloc(size_t size);
void free(void *ptr);
void* aligned_malloc(size_t required_bytes, size_t alignment);
void aligned_free(void *p);

}

size_t TotalMem();
size_t AllocatedMem();
void AddCluster(void *cluster, size_t size);

#endif	// _MALLOC_H_
