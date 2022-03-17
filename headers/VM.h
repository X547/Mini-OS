#ifndef _VM_H_
#define _VM_H_

#include <stdint.h>
#include <stddef.h>
#include "RiscV.h"
#include "Lists.h"
#include "AutoDeleter.h"

namespace VM {

enum {
	userBase = 0,
	userSize = 0x4000000000,
	kernelBase = 0xffffffc000000000,
	kernelSize = 0x4000000000
};

enum {
	readFlag,
	writeFlag,
	execFlag,
	userFlag,
};

class PageAllocator {
private:
	ArrayDeleter<uint32_t> fAllocSet;
	size_t fPageCnt;

public:
	PageAllocator(size_t pageCnt);
	~PageAllocator();

	bool Alloc(uint64_t &page);
	bool AllocMultiple(uint64_t *pages, size_t cnt);
	bool AllocContiguous(uint64_t &begPage, size_t cnt);
	bool AllocAt(uint64_t begPage, size_t cnt);
	bool Free(uint64_t page);
	bool FreeMultiple(uint64_t *pages, size_t cnt);
	bool FreeContiguous(uint64_t begPage, size_t cnt);
};

struct FreePteList: public Lists::List {
	Pte *pte;
};

class PageTable {
private:
	Pte *fRoot;
	FreePteList fFreePtes;

	Pte *AllocPte();
	void DumpInt(Pte *pte, uint64_t virtAdr, uint32_t level);
	
	friend void SetPageTable(PageTable *pt);

public:
	PageTable();
	~PageTable();

	Pte *This(uint64_t virtAdr, bool alloc);
	bool Map(uint64_t virtAdr, uint64_t physAdr, uint32_t flags);
	bool Unmap(uint64_t virtAdr);
	bool MapRange(uint64_t virtAdr, uint64_t physAdr, size_t len, uint32_t flags);
	bool UnmapRange(uint64_t virtAdr, size_t len);
	void Dump();
};

void Init();
void SetPageTable(PageTable *pt);

}

#endif	// _VM_H_
