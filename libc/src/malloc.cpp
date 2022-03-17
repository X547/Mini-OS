#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h> // abort()
#include <new>
#include "Lists.h"
#include "Graphics.h"


struct Block {
	size_t size; // bit 0: allocated flag
	union {
		size_t allocated;
		Lists::List free;
	};
};

Lists::List gFreeBlocks = {&gFreeBlocks, &gFreeBlocks};
size_t gTotalMem = 0, gAllocatedMem = 0;

size_t TotalMem()
{
	return gTotalMem;
}

size_t AllocatedMem()
{
	return gAllocatedMem;
}

void AddCluster(void *cluster, size_t size)
{
	gTotalMem += size;
	gAllocatedMem += size;
	Block *blk = (Block*)cluster;
	blk->size = size | 1;
  free((void*)&blk->allocated);
}

void *malloc(size_t size)
{
	// WriteString("malloc("); WriteInt(size); WriteString(")"); WriteLn();
	size = RoundUp(size, 8);
	size += sizeof(size_t);
	if (size < sizeof(Block)) size = sizeof(Block);
	for (Lists::List *it = gFreeBlocks.fNext; it != &gFreeBlocks; it = it->fNext) {
		Block *blk = BasePtr(it, Block, free);
		Assert(!(blk->size & 1));
		if (blk->size >= size) {
			if (blk->size - size >= sizeof(Block)) {
				Block *blk2 = (Block*)((uint8_t*)blk + size);
				blk2->size = blk->size - size;
#if 1
				gFreeBlocks.InsertAfter(&(blk2->free));
#else
				gAllocatedMem += blk2->size;
				blk2->size |= 1;
				free((void*)&(blk2->allocated));
#endif
				blk->size = size | 1;
			} else {
				blk->size |= 1;
 			}
 			gFreeBlocks.Remove(it);
			gAllocatedMem += blk->size & ~(size_t)1;
			return (void*)&(blk->allocated);
		}
	}
	return NULL;
}

void free(void *ptr)
{
	// WriteString("free(0x"); WriteHex((size_t)ptr, 8); WriteString(")"); WriteLn();
	if (ptr == NULL) return;
	Assert((size_t)ptr >= 0x80000000); // illegal pointer
	Block *blk = BasePtr(ptr, Block, allocated);
	Assert(blk->size & 1); // double free
	blk->size &= ~(size_t)1;
	gAllocatedMem -= blk->size;
	gFreeBlocks.InsertAfter(&(blk->free));
}

void *aligned_malloc(size_t required_bytes, size_t alignment)
{
	void *p1; // original block
	void **p2; // aligned block
	int offset = alignment - 1 + sizeof(void*);
	if ((p1 = (void*)malloc(required_bytes + offset)) == NULL) {
		return NULL;
	}
	p2 = (void**)(((size_t)(p1) + offset) & ~(alignment - 1));
	p2[-1] = p1;
	return p2;
}

void aligned_free(void *p)
{
	free(((void**)p)[-1]);
}
