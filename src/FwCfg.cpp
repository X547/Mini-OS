#include "FwCfg.h"
#include "Graphics.h"
#include "Htif.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>


FwCfgRegs *volatile gFwCfgRegs;


#define _B(n)	((unsigned long long)((uint8_t *)&x)[n])
static inline uint16_t SwapEndian16(uint16_t x)
{
	return (_B(0) << 8) | _B(1);
}

static inline uint32_t SwapEndian32(uint32_t x)
{
	return (_B(0) << 24) | (_B(1) << 16) | (_B(2) << 8) | _B(3);
}

static inline uint64_t SwapEndian64(uint64_t x)
{
	return (_B(0) << 56) | (_B(1) << 48) | (_B(2) << 40) | (_B(3) << 32)
		| (_B(4) << 24) | (_B(5) << 16) | (_B(6) << 8) | _B(7);
}
#undef _B


namespace FwCfg {

void Select(uint16_t selector)
{
	gFwCfgRegs->selector = SwapEndian16(selector);
}

void DmaOp(uint8_t *bytes, size_t count, uint32_t op)
{
	FwCfgDmaAccess volatile dma;
	dma.control = SwapEndian32(1 << op);
	dma.length = SwapEndian32(count);
	dma.address = SwapEndian64((size_t)bytes);
	gFwCfgRegs->dmaAdr = SwapEndian64((size_t)&dma);
	while (uint32_t control = SwapEndian32(dma.control) != 0) {
		if (((1 << fwCfgDmaFlagsError) & control) != 0)
			abort();
	}
}

void ReadBytes(uint8_t *bytes, size_t count)
{
	DmaOp(bytes, count, fwCfgDmaFlagsRead);
}

void WriteBytes(uint8_t *bytes, size_t count)
{
	DmaOp(bytes, count, fwCfgDmaFlagsWrite);
}

uint8_t  Read8 () {uint8_t  val; ReadBytes(          &val, sizeof(val)); return val;}
uint16_t Read16() {uint16_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}
uint32_t Read32() {uint32_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}
uint64_t Read64() {uint64_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}


void ListDir()
{
	uint32_t count = SwapEndian32(Read32());
	WriteString("count: "); WriteInt(count); WriteLn();
	for (uint32_t i = 0; i < count; i++) {
		FwCfgFile file;
		ReadBytes((uint8_t*)&file, sizeof(file));
		file.size = SwapEndian32(file.size);
		file.select = SwapEndian16(file.select);
		file.reserved = SwapEndian16(file.reserved);
		WriteLn();
		WriteString("size: "); WriteInt(file.size); WriteLn();
		WriteString("select: "); WriteInt(file.select); WriteLn();
		WriteString("reserved: "); WriteInt(file.reserved); WriteLn();
		WriteString("name: "); WriteString(file.name); WriteLn();
	}
}

bool ThisFile(FwCfgFile& file, uint16_t dir, const char *name)
{
	Select(dir);
	uint32_t count = SwapEndian32(Read32());
	for (uint32_t i = 0; i < count; i++) {
		ReadBytes((uint8_t*)&file, sizeof(file));
		file.size = SwapEndian32(file.size);
		file.select = SwapEndian16(file.select);
		file.reserved = SwapEndian16(file.reserved);
		if (strcmp(file.name, name) == 0)
			return true;
	}
	return false;
}

void InitFramebuffer()
{
	FwCfgFile file;
	if (!ThisFile(file, fwCfgSelectFileDir, "etc/ramfb")) {
		WriteString("[!] ramfb not found"); WriteLn();
		return;
	}
	WriteString("file.select: "); WriteInt(file.select); WriteLn();

	RamFbCfg cfg;
	uint32_t width = 640, height = 480;

	gFrontBuf.colors = (uint32_t*)malloc(4*width*height);
	gFrontBuf.stride = width;
	gFrontBuf.width = width;
	gFrontBuf.height = height;

	cfg.addr = SwapEndian64((size_t)gFrontBuf.colors);
	cfg.fourcc = SwapEndian32(ramFbFormatXrgb8888);
	cfg.flags = SwapEndian32(0);
	cfg.width = SwapEndian32(width);
	cfg.height = SwapEndian32(height);
	cfg.stride = SwapEndian32(4*width);
	Select(file.select);
	WriteBytes((uint8_t*)&cfg, sizeof(cfg));
}

void Init()
{
	WriteString("gFwCfgRegs: 0x"); WriteHex((size_t)gFwCfgRegs, 8); WriteLn();
	if (gFwCfgRegs == NULL)
		return;
	Select(fwCfgSelectSignature);
	WriteString("fwCfgSelectSignature: 0x"); WriteHex(Read32(), 8); WriteLn();
	Select(fwCfgSelectId);
	WriteString("fwCfgSelectFileDir: 0x"); WriteHex(Read32(), 8); WriteLn();
	Select(fwCfgSelectFileDir);
	ListDir();
	InitFramebuffer();
}

};
