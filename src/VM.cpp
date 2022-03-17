#include "VM.h"

#include <string.h>
#include <new>
#include "Graphics.h"
#include "Htif.h"

extern size_t gMemorySize;

enum {
	memoryBase = 0x80000000,
};

namespace VM {

PageAllocator *gPhysAlloc;
PageAllocator *gVirtAlloc;


static Pte *PteFromPhysAdr(uint64_t physAdr) {return (Pte*)physAdr;}
static uint64_t PhysAdrFromPte(Pte *pte) {return (uint64_t)pte;}
static uint64_t SignExtendVirtAdr(uint64_t virtAdr)
{
	if (((uint64_t)1 << 38) & virtAdr)
		return virtAdr | 0xFFFFFF8000000000;
	return virtAdr;
}

void WritePteFlags(uint32_t flags)
{
	bool first = true;
	WriteString("{");
	for (int32_t i = 0; i < 32; i++) {
		if ((1 << i) & flags) {
			if (first) first = false; else WriteString(", ");
			switch (i) {
			case pteValid:    WriteString("valid"); break;
			case pteRead:     WriteString("read"); break;
			case pteWrite:    WriteString("write"); break;
			case pteExec:     WriteString("exec"); break;
			case pteUser:     WriteString("user"); break;
			case pteGlobal:   WriteString("global"); break;
			case pteAccessed: WriteString("accessed"); break;
			case pteDirty:    WriteString("dirty"); break;
			default:          WriteInt(i);
			}
		}
	}
	WriteString("}");
}


//#pragma mark PageAllocator

PageAllocator::PageAllocator(size_t pageCnt): fPageCnt(pageCnt)
{
	fAllocSet.SetTo(new(std::nothrow) uint32_t[(fPageCnt + 31)/32]);
	memset(fAllocSet.Get(), 0, (fPageCnt + 31)/32*4);
}

PageAllocator::~PageAllocator()
{
}

bool PageAllocator::Alloc(uint64_t &page)
{
	for (size_t i = 0; i < (fPageCnt + 31)/32; i++) {
		if (fAllocSet[i] != 0xffffffff) {
			for (int j = 0; j < 32; j++) {
				if (((1 << j) & fAllocSet[i]) == 0) {
					fAllocSet[i] |= (1 << j);
					page = 32*i + j;
					return true;
				}
			}
		}
	}
	return false;
}

bool PageAllocator::AllocMultiple(uint64_t *pages, size_t cnt)
{
	for (size_t i = 0; i < cnt; i++) {
		if (!Alloc(pages[i])) {
			FreeMultiple(pages, i);
			return false;
		}
	}
	return true;
}

bool PageAllocator::Free(uint64_t page)
{
	if (page >= fPageCnt) return false;
	uint64_t i = page/32, j = page % 32;
	if (((1 << j) & fAllocSet[i]) == 0) return false;
	fAllocSet[i] &= ~(1 << j);
	return true;
}

bool PageAllocator::FreeMultiple(uint64_t *pages, size_t cnt)
{
	for (size_t i = 0; i < cnt; i++) {
		if (!Free(pages[i]))
			return false;
	}
	return true;
}


//#pragma mark PageTable

PageTable::PageTable()
{
	fFreePtes.fPrev = &fFreePtes; fFreePtes.fNext = &fFreePtes;
	fRoot = PteFromPhysAdr((size_t)aligned_malloc(pageSize, pageSize));
	memset(fRoot, 0, pageSize);
}

PageTable::~PageTable()
{
}


Pte *PageTable::This(uint64_t virtAdr, bool alloc)
{
	Pte *pte = fRoot;
	for (int level = 2; level > 0; level --) {
		pte += PhysAdrPte(virtAdr, level);
		if (!((1 << pteValid) & pte->flags)) {
			if (!alloc) return NULL;
			pte->ppn = ((size_t)aligned_malloc(pageSize, pageSize))/pageSize;
			if (pte->ppn == 0) return NULL;
			memset(PteFromPhysAdr(pageSize*pte->ppn), 0, pageSize);
			pte->flags |= (1 << pteValid);
		}
		pte = PteFromPhysAdr(pageSize*pte->ppn);
	}
	pte += PhysAdrPte(virtAdr, 0);
	return pte;
}

bool PageTable::Map(uint64_t virtAdr, uint64_t physAdr, uint32_t flags)
{
	Pte *pte = This(virtAdr, true);
	if (pte == NULL) return false;
	pte->ppn = physAdr/pageSize;
	pte->flags |= (1 << pteValid);
	if ((1 << readFlag)  & flags) pte->flags |= (1 << pteRead);
	if ((1 << writeFlag) & flags) pte->flags |= (1 << pteWrite);
	if ((1 << execFlag)  & flags) pte->flags |= (1 << pteExec);
	if ((1 << userFlag)  & flags) pte->flags |= (1 << pteUser);
	FlushTlbPage(virtAdr);
	return true;
}

bool PageTable::Unmap(uint64_t virtAdr)
{
	Pte *pte = This(virtAdr, false);
	if (pte == NULL) return false;
	pte->flags &= ~pteValid;
	FlushTlbPage(virtAdr);
	return true;
}

bool PageTable::MapRange(uint64_t virtAdr, uint64_t physAdr, size_t len, uint32_t flags)
{
	for (size_t i = 0; i < len; i += pageSize) {
		if (!Map(virtAdr + i, physAdr + i, flags)) {
			UnmapRange(virtAdr, i);
			return false;
		}
	}
	return true;
}

bool PageTable::UnmapRange(uint64_t virtAdr, size_t len)
{
	for (size_t i = 0; i < len; i += pageSize) {
		if (!Unmap(virtAdr + i)) return false;
	}
	return true;
}

void PageTable::DumpInt(Pte *pte, uint64_t virtAdr, uint32_t level)
{
	for (uint32_t i = 0; i < pteCount; i++) {
		if ((1 << pteValid) & pte[i].flags) {
			if (level == 0) {
				WriteTab(); WriteString("0x"); WriteHex(SignExtendVirtAdr(virtAdr + pageSize*i), 8); WriteString(": 0x");
				WriteHex(pte[i].ppn * pageSize, 8); WriteString(", "); WritePteFlags(pte[i].flags); WriteLn();
			} else {
				DumpInt(PteFromPhysAdr(pageSize*pte[i].ppn), virtAdr + ((uint64_t)i << (pageBits + pteIdxBits*level)), level - 1);
			}
		}
	}
}

void PageTable::Dump()
{
	WriteString("PageTable:"); WriteLn();
	DumpInt(fRoot, 0, 2);
}


//#pragma mark -

void Init()
{
	WriteString("VM::Init()"); WriteLn();
//	gPhysAlloc = new(std::nothrow) PageAllocator((gMemorySize + pageSize - 1)/pageSize);
/*
	uint64_t page = (uint64_t)(-1);
	for (int i = 0; i < 1000; i++) {
		gPhysAlloc->Alloc(page); WriteString("Alloc(): "); WriteInt(page); WriteLn();
	}
	WriteString("Free"); WriteLn();
	for (int i = 0; i < 100; i++) {
		gPhysAlloc->Free(500 + i);
	}
	for (int i = 0; i < 1000; i++) {
		gPhysAlloc->Alloc(page); WriteString("Alloc(): "); WriteInt(page); WriteLn();
	}
*/

	PageTable *pageTable = new(std::nothrow) PageTable();
	
/*
	uint64_t virtAdr = (uint64_t)1 << 38;
	for (int32_t level = 2; level >= 0; level --) {
		WriteInt(PhysAdrPte(virtAdr, level)); WriteString(" ");
	}
	WriteLn();
	pageTable->Map(0xfffffffffffff000, 0x80000000, (1 << readFlag) | (1 << writeFlag));
	pageTable->Map(kernelBase, 0x90000000, (1 << readFlag) | (1 << writeFlag));
	pageTable->Map(kernelBase + pageSize, 0x100000000, (1 << readFlag) | (1 << writeFlag));

	pageTable->Dump();
	Shutdown();
*/
	
	pageTable->MapRange(memoryBase, memoryBase, gMemorySize, (1 << readFlag) | (1 << writeFlag) | (1 << execFlag));
	// CLINT
	pageTable->MapRange(0x2000000, 0x2000000, 0xC0000, (1 << readFlag) | (1 << writeFlag));
	// HTIF
	pageTable->MapRange(0x40008000, 0x40008000, 0x1000, (1 << readFlag) | (1 << writeFlag));
	// PLIC
	pageTable->MapRange(0x40100000, 0x40100000, 0x400000, (1 << readFlag) | (1 << writeFlag));
	// virtio
	pageTable->MapRange(0x40010000, 0x40010000, 0x1000 * 16, (1 << readFlag) | (1 << writeFlag));
	// framebuffer
	pageTable->MapRange(0x41000000, 0x41000000, 0x300000, (1 << readFlag) | (1 << writeFlag));
/*
	pageTable->Map(0x1000, 0x2000, (1 << readFlag) | (1 << writeFlag));
	pageTable->Map(0x10000000, 0x3000, (1 << readFlag) | (1 << writeFlag));
	pageTable->Map(0x20000000, 0x4000, (1 << readFlag) | (1 << writeFlag));
	pageTable->Map(0x30000000, 0x5000, (1 << readFlag) | (1 << writeFlag));
*/
	//pageTable->Dump();
	SetPageTable(pageTable);
}

void SetPageTable(PageTable *pt)
{
	if (pt == NULL) {
		SetSatp(0);
	} else {
		SatpReg satp;
		satp.ppn = PhysAdrFromPte(pt->fRoot)/pageSize;
		satp.asid = 0;
		satp.mode = satpModeSv39;
		SetSatp(satp.val);
		FlushTlbAll();
	}
}

}

/*
                            level 2   level 1   level 0       offset
	user
0000000000000000000000000 000000000 000000000 000000000 000000000000  0x0000000000000000
...
0000000000000000000000000 011111111 111111111 111111111 111111111111  0x0000003fffffffff

	kernel
1111111111111111111111111 100000000 000000000 000000000 000000000000  0xffffffc000000000
...
1111111111111111111111111 111111111 111111111 111111111 111111111111  0xffffffffffffffff
*/
