#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <new>

#include "Atomic.h"
#include "AutoDeleter.h"
#include "Graphics.h"
#include "Threads.h"
#include "Timers.h"
#include "Modules.h"
#include "Traps.h"
#include "VM.h"
#include "RiscV.h"
#include "Htif.h"
#include "FwCfg.h"
#include "Plic.h"
#include "Virtio.h"
#include "Views.h"
#include "Buttons.h"

#include <libfdt.h>


extern uint8_t startupBase[], startupEnd[];
extern uint8_t startupBssBase[], startupBssEnd[];
size_t gMemorySize = 0;

enum {
	memoryBase = 0x80000000,
};


void AllocTest()
{
	enum {
		blockSize = 256,
	};
	struct Block {
		Block *next;
		uint8_t data[blockSize - sizeof(void*)];
	};
	Block *blocks = NULL, *b;
	size_t totalSize = 0, blockCnt = 0;
	for (;;) {
		b = new(std::nothrow) Block();
		if (b == 0) break;
		b->next = blocks; blocks = b;
		totalSize += sizeof(Block);
		blockCnt++;
	}
	WriteString("totalSize: "); WriteInt(totalSize); WriteString(", ");
	WriteString("blockCnt: "); WriteInt(blockCnt); WriteLn();
	while (blocks != NULL) {
		b = blocks; blocks = blocks->next;
		delete b;
	}
}


class TestThread
{
private:
	Threads::Thread *fThread;
	int32_t fX, fY;
	int32_t fCnt;

	static int32_t Entry(void *arg)
	{
/*
		WriteLn();
		ThreadDump();
		WriteString("mstatus: "); WriteMstatus(Mstatus()); WriteLn();
*/
		TestThread &th = *(TestThread*)arg;
		int i = 0;
	
		for (;/*i < th.fCnt*/;) {
			Point pos(th.fX, th.fY + gFont.asc);
			char str[9];
			{
				int n = i;
				str[8] = '\0';
				for (int j = 7; j >= 0; j--) {str[j] = '0' + (n % 10);n /= 10;}
			}
			int32_t l, t, r, b;
			l = th.fX; t = th.fY;
			r = l + StringWidth(str, gFont); b = t + gFont.asc + gFont.dsc;
			//Clear(gBackBuf, 0xffffffff);
			FillRect(gBackBuf, Rect(l, t, r, b), 0xffffffff);
			DrawString(gBackBuf, pos, 0xff000000, str, gFont);
			FlushBackBuf(Rect(l, t, r, b));
			
			i++;
//			ThreadYield();
		}
/*
		WriteLn();
		WriteString("-"); WriteString(th.fThread->Name()); WriteLn();
		ThreadDump();
*/
		return 123;
	}

public:
	TestThread(const char *name, int32_t x, int32_t y, int32_t cnt): fX(x), fY(y), fCnt(cnt)
	{
		fThread = new Threads::Thread(name, 1024*1024, Entry, this);
	}
	
	Threads::Thread *ThisThread() {return fThread;}
};

static int32_t IdleThreadEntry(void *arg)
{
	for(;;) {
/*
		WriteString("IdleThreadEntry()"); WriteLn();
		WriteString("mstatus: "); WriteMstatus(Mstatus()); WriteLn();
*/
		Threads::Yield();
		//SetMstatus(Mstatus() | mstatusMie);
		//Wfi();
	}
	return 0;
}


class TestTimer: public Timers::Timer
{
public:
	TestTimer()
	{}
	
	void Do()
	{
		WriteHex((size_t)this, 8); WriteString(".TestTimer::Do()"); WriteLn();
	}
	
};

template <typename Base>
static inline Base CheckNull(Base ptr) {if (!ptr) {abort();} return ptr;}

static uint32_t GetInterrupt(const void *fdt, int node)
{
	if (auto prop = (uint32_t*)fdt_getprop(fdt, node, "interrupts-extended", NULL)) {
		return fdt32_to_cpu(*(prop + 1));
	}
	if (auto prop = (uint32_t*)fdt_getprop(fdt, node, "interrupts", NULL)) {
		return fdt32_to_cpu(*prop);
	}
	WriteString("[!] no interrupt field"); WriteLn();
	return 0;
}

void HandleFdt(const void *fdt, int node, uint32_t addressCells, uint32_t sizeCells, uint32_t interruptCells /* from parent node */)
{
	const char *device_type = (const char*)fdt_getprop(fdt, node, "device_type", NULL);
	if (device_type != NULL && strcmp(device_type, "memory") == 0) {
		gMemorySize = fdt64_to_cpu(*((uint64_t*)fdt_getprop(fdt, node, "reg", NULL) + 1));
		return;
	}
	const char *compatible = (const char*)fdt_getprop(fdt, node, "compatible", NULL);
	if (compatible == NULL) return;
	if (strcmp(compatible, "ucb,htif0") == 0) {
		gHtifRegs = (HtifRegs *volatile)0x40008000; // no "reg" prop
	} else if (strcmp(compatible, "riscv,clint0") == 0) {
		Timers::gClintRegs = (Timers::ClintRegs *volatile)fdt64_to_cpu(*(uint64_t*)fdt_getprop(fdt, node, "reg", NULL));
	} else if (strcmp(compatible, "riscv,plic0") == 0) {
		gPlicRegs = (PlicRegs *volatile)fdt64_to_cpu(*(uint64_t*)fdt_getprop(fdt, node, "reg", NULL));
	} else if (strcmp(compatible, "virtio,mmio") == 0) {
		Virtio::Register(
			(void*)fdt64_to_cpu(*((uint64_t*)fdt_getprop(fdt, node, "reg", NULL) + 0)),
			fdt64_to_cpu(*((uint64_t*)fdt_getprop(fdt, node, "reg", NULL) + 1)),
			GetInterrupt(fdt, node)
		);
	} else if (strcmp(compatible, "simple-framebuffer") == 0) {
		gFrontBuf.colors = (uint32_t*)fdt64_to_cpu(*(uint64_t*)fdt_getprop(fdt, node, "reg", NULL));
		gFrontBuf.stride = fdt32_to_cpu(*(uint32_t*)fdt_getprop(fdt, node, "stride", NULL))/4; 
		gFrontBuf.width = fdt32_to_cpu(*(uint32_t*)fdt_getprop(fdt, node, "width", NULL));
		gFrontBuf.height = fdt32_to_cpu(*(uint32_t*)fdt_getprop(fdt, node, "height", NULL)); 
	} else if (strcmp(compatible, "qemu,fw-cfg-mmio") == 0) {
		gFwCfgRegs = (FwCfgRegs *volatile)fdt64_to_cpu(*(uint64_t*)fdt_getprop(fdt, node, "reg", NULL));
	}
}

void TraverseFdt(const void *fdt, int &node, int &depth)
{
	int curDepth = depth;
	for (int i = 0; i < depth; i++) WriteString("  ");
	WriteString("node('");
	WriteString(fdt_get_name(fdt, node, NULL));
	WriteString("')"); WriteLn();
	node = fdt_next_node(fdt, node, &depth);
	while (node >= 0 && depth == curDepth + 1) {
		TraverseFdt(fdt, node, depth);
	}
}

void ProcessFdt(const void *fdt)
{
	if (fdt == NULL) {
		WriteString("fdt == NULL"); WriteLn();
		abort();
	}
	int err = fdt_check_header(fdt);
	if (err) {
		WriteString("fdt error: ");
		WriteString(fdt_strerror(err));
		WriteLn();
		abort();
	}
	int node = -1;
	int depth = -1;
	while ((node = fdt_next_node(fdt, node, &depth)) >= 0 && depth >= 0) {
		HandleFdt(fdt, node, 2, 2, 2);
	}
}

void DumpFdt(const void *fdt)
{
	if (!fdt)
		return;

	int err = fdt_check_header(fdt);
	if (err) {
		WriteString("fdt error: ");
		WriteString(fdt_strerror(err));
		WriteLn();
		return;
	}

	WriteString("fdt tree:"); WriteLn();

	int node = -1;
	int depth = -1;
	while ((node = fdt_next_node(fdt, node, &depth)) >= 0 && depth >= 0) {
		for (int i = 0; i < depth; i++) WriteString("  ");
		// WriteInt(node); WriteString(", "); WriteInt(depth); WriteString(": ");
		WriteString("node('");
		WriteString(fdt_get_name(fdt, node, NULL));
		WriteString("')"); WriteLn();
		depth++;
		for (int prop = fdt_first_property_offset(fdt, node); prop >= 0; prop = fdt_next_property_offset(fdt, prop)) {
			int len;
			const struct fdt_property *property = fdt_get_property_by_offset(fdt, prop, &len);
			if (property == NULL) {
				for (int i = 0; i < depth; i++) WriteString("  ");
				WriteString("getting prop at ");
				WriteInt(prop);
				WriteString(": ");
				WriteString(fdt_strerror(len));
				WriteLn();
				break;
			}
			for (int i = 0; i < depth; i++) WriteString("  ");
			WriteString("prop('");
			WriteString(fdt_string(fdt, fdt32_to_cpu(property->nameoff)));
			WriteString("'): ");
			if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "compatible") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "model") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "status") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "device_type") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "riscv,isa") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "mmu-type") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "format") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "bootargs") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "stdout-path") == 0
			) {
				WriteString("'");
				WriteString((const char*)property->data);
				WriteString("'");
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "reg") == 0) {
				if (fdt32_to_cpu(property->len) == 16) {
				WriteString("0x"); WriteHex(fdt64_to_cpu(*(uint64_t*)property->data), 0);
				WriteString(", ");
				WriteString("0x"); WriteHex(fdt64_to_cpu(*((uint64_t*)(property->data) + 1)), 0);
				} else if (fdt32_to_cpu(property->len) == 4) {
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(uint32_t*)property->data), 0);
				}
			} else if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "phandle") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "clock-frequency") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "timebase-frequency") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "#address-cells") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "#size-cells") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "#interrupt-cells") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupts") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupt-parent") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "riscv,ndev") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "value") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "offset") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "regmap") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "bank-width") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "width") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "height") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "stride") == 0
			) {
				WriteInt(fdt32_to_cpu(*(uint32_t*)property->data));
			} else if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupts-extended") == 0
			) {
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 2) {
					if (it != (uint32_t*)property->data) WriteString(", ");
					WriteString("(");
					WriteInt(fdt32_to_cpu(*it));
					WriteString(", ");
					WriteInt(fdt32_to_cpu(*(it + 1)));
					WriteString(")");
				}
			} else if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "ranges") == 0
			) {
				WriteLn();
				depth++;
				// kind
				// child address
				// parent address
				// size
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 7) {
					for (int i = 0; i < depth; i++) WriteString("  ");
					uint32_t kind = fdt32_to_cpu(*(it + 0));
					switch (kind & 0x03000000) {
					case 0x00000000: WriteString("CONFIG"); break;
					case 0x01000000: WriteString("IOPORT"); break;
					case 0x02000000: WriteString("MMIO"); break;
					case 0x03000000: WriteString("MMIO_64BIT"); break;
					}
					WriteString(" (0x"); WriteHex(kind, 8);
					WriteString("), ");
					WriteString("child: 0x"); WriteHex(fdt64_to_cpu(*(uint64_t*)(it + 1)), 8);
					WriteString(", ");
					WriteString("parent: 0x"); WriteHex(fdt64_to_cpu(*(uint64_t*)(it + 3)), 8);
					WriteString(", ");
					WriteString("len: 0x"); WriteHex(fdt64_to_cpu(*(uint64_t*)(it + 5)), 8);
					WriteLn();
				}
				for (int i = 0; i < depth; i++) WriteString("  ");
				depth--;
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "bus-range") == 0) {
				uint32_t *it = (uint32_t*)property->data;
				WriteInt(fdt32_to_cpu(*it));
				WriteString(", ");
				WriteInt(fdt32_to_cpu(*(it + 1)));
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupt-map-mask") == 0) {
				WriteLn();
				depth++;
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it++) {
					for (int i = 0; i < depth; i++) WriteString("  ");
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(uint32_t*)it), 8);
					WriteLn();
				}
				for (int i = 0; i < depth; i++) WriteString("  ");
				depth--;
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupt-map") == 0) {
				WriteLn();
				depth++;
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 6) {
					for (int i = 0; i < depth; i++) WriteString("  ");
					// child unit address
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(it + 0)), 8);
					WriteString(", ");
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(it + 1)), 8);
					WriteString(", ");
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(it + 2)), 8);
					WriteString(", ");
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(it + 3)), 8);

					WriteString(", bus: "); WriteInt(fdt32_to_cpu(*(it + 0)) / (1 << 16) % (1 << 8));
					WriteString(", dev: "); WriteInt(fdt32_to_cpu(*(it + 0)) / (1 << 11) % (1 << 5));
					WriteString(", fn: "); WriteInt(fdt32_to_cpu(*(it + 0)) % (1 << 3));

					WriteString(", childIrq: ");
					// child interrupt specifier
					WriteInt(fdt32_to_cpu(*(it + 3)));
					WriteString(", parentIrq: (");
					// interrupt-parent
					WriteInt(fdt32_to_cpu(*(it + 4)));
					WriteString(", ");
					WriteInt(fdt32_to_cpu(*(it + 5)));
					WriteString(")");
					WriteLn();
					if (((it - (uint32_t*)property->data) / 6) % 4 == 3 && ((uint8_t*)(it + 6) - (uint8_t*)property->data < fdt32_to_cpu(property->len)))
						WriteLn();
				}
				for (int i = 0; i < depth; i++) WriteString("  ");
				depth--;
			} else {
				WriteString("?");
			}
			WriteString(" (len ");
			WriteInt(fdt32_to_cpu(property->len));
			WriteString(")"); WriteLn();
/*
			dump_hex(property->data, fdt32_to_cpu(property->len), depth);
*/
		}
		depth--;
	}
}

struct __attribute__((packed)) PciConfig {
	uint16_t vendorId;
	uint16_t deviceId;
	uint16_t command;
	uint16_t status;
	uint32_t revisionId : 8;
	uint32_t classCode : 24;
	uint8_t cacheLineSize;
	uint8_t latTimer;
	uint8_t headerType;
	uint8_t bist;
	uint32_t bars[6];
	uint32_t cardbusCisPtr;
	uint16_t subsystemVendorId;
	uint16_t subsystemId;
	uint32_t expRomBaseAdr;
	uint32_t capPtr : 8;
	uint32_t reserved1 : 24;
	uint32_t reserved2;
	uint8_t irqLine;
	uint8_t irqPin;
	uint8_t minGnt;
	uint8_t maxLat;
};

#define PCIE_VADDR(base, bus, slot, func, reg)  ((base) + \
	((((bus) & 0xff) << 20) | (((slot) & 0x1f) << 15) |   \
    (((func) & 0x7) << 12) | ((reg) & 0xfff)))

void DumpPci()
{
	// nec-usb-xhci: (vendor: 0x1033, device: 0x0194, revision: 0x03)

	auto pciRegs = (uint32_t *volatile)0x30000000;	
	PciConfig cfg;
	
	memcpy(&cfg, (void*)PCIE_VADDR(0x30000000, 0, 1, 0, 0), sizeof(PciConfig));
	
	WriteString("vendorId: 0x"); WriteHex(cfg.vendorId, 4); WriteLn();
	WriteString("deviceId: 0x"); WriteHex(cfg.deviceId, 4); WriteLn();
	WriteString("command: 0x"); WriteHex(cfg.command, 4); WriteLn();
	WriteString("status: 0x"); WriteHex(cfg.status, 4); WriteLn();
	WriteString("revisionId: 0x"); WriteHex(cfg.revisionId, 2); WriteLn();
	WriteString("classCode: 0x"); WriteHex(cfg.status, 6); WriteLn();
}


class ColorView: public Views::View
{
private:
	uint32_t fColor;

public:
	ColorView(uint32_t color): fColor(color) {}

	void SetColor(uint32_t color)
	{
		if (fColor == color) return;
		fColor = color;
		Invalidate(Bounds());
	}

	void Draw(RasBufOfs32 rb, Rect dirty) override
	{
		FillRect(rb, Bounds(), fColor);
	}
};

class TestView: public Views::View
{
private:
	bool fMainFocus, fPointerFocus;
	uint32_t fOldPointerBtns;

public:
	TestView(): fMainFocus(false), fPointerFocus(false), fOldPointerBtns(0) {}
	
	void Draw(RasBufOfs32 rb, Rect dirty) override
	{
		uint32_t c2 = 0xff999999, c1 = 0xffffffff;
		if (fPointerFocus) {c1 = 0xffbbddff; c2 = 0xff0099ff;}
		FillRect(rb, Bounds(), c1);
		StrokeRect(rb, Bounds(), 2, c2);
		if (fMainFocus)
			StrokeRect(rb, Bounds().Inset(3, 3), 1, 0xffff0000);
	}

	void HandleMsg(Views::Message &_msg) override
	{
		if (auto msg = dynamic_cast<Views::FocusMsg*>(&_msg)) {
			if (msg->focus == Views::pointerFocus) {
				Invalidate(Bounds());
				if (!msg->on) {
					fPointerFocus = false;
					fOldPointerBtns = 0;
				}
			} else if (msg->focus == Views::mainFocus) {
				Invalidate(Bounds());
				fMainFocus = msg->on;
			}
		} else if (auto msg = dynamic_cast<Views::PointerMsg*>(&_msg)) {
			if (!fPointerFocus) {
				fPointerFocus = true;
			} else {
				if (msg->btns & ~fOldPointerBtns)
					GetPort()->SetFocus(Views::mainFocus, this);
			}
			fOldPointerBtns = msg->btns;
		}
	}
};

class PatternView: public Views::View
{
private:
	bool fFocus;
	Point fPos;
	
public:
	PatternView(): fFocus(false) {SetCursor(NULL);}
	
	void Draw(RasBufOfs32 rb, Rect dirty) override
	{
		if (fFocus) {
			//rb = rb.ClipOfs(Rect(32, 32) - Point(16, 16) + fPos);
			//rb.pos += fPos - Point(Bounds().Width()/2, Bounds().Height()/2);
		}
		
		ArrayDeleter<uint32_t> data(new(std::nothrow) uint32_t[Bounds().Width()*Bounds().Height()]);
		RasBufOfs32 rb2;
		rb2.colors = data.Get();
		rb2.stride = Bounds().Width();
		rb2.width = Bounds().Width();
		rb2.height = Bounds().Height();
		rb2.pos = Point(0, 0);
		Clear(rb2, 0);
		
		Rect rc = Bounds();
		int i = 0;
		uint32_t c;
		while (rc.l < rc.r & rc.t < rc.b) {
			switch (i) {
			case 0: c = 0xffff0000; break;
			case 1: c = 0xff00ff00; break;
			case 2: c = 0xff0000ff; break;
			}
			StrokeRect(rb2, rc, 1, c);
			rc.l += 3; rc.t += 3; rc.r -= 3; rc.b -= 3;
			i = (i + 1) % 3;
		}
		
		if (fFocus) {
			rb2 = rb2.ClipOfs(Rect(32, 32) - Point(16, 16) + fPos);
			//rb2.pos = -rb2.pos;
			//rb2.pos += fPos - Point(Bounds().Width()/2, Bounds().Height()/2);
		}
		
		BlitRgb(rb, rb2);
	}

	void HandleMsg(Views::Message &_msg) override
	{
		if (auto msg = dynamic_cast<Views::FocusMsg*>(&_msg)) {
			if (!msg->on) {
				fFocus = false;
				Invalidate(Bounds());
			}
		} else if (auto msg = dynamic_cast<Views::PointerMsg*>(&_msg)) {
			fFocus = true;
			fPos = msg->pos;
			Invalidate(Bounds());
		}
	}
};

class PointerTestView: public Views::View
{
private:
	bool fFocus;
	Point fPos;
	uint32_t fOldPointerBtns;
	
public:
	PointerTestView(): fFocus(false) {SetCursor(NULL);}
	
	void Draw(RasBufOfs32 rb, Rect dirty) override
	{
		StrokeRect(rb, Bounds(), 1, 0xffcccccc);
		if (fFocus) {
			for (int i = -6; i <= 6; i++)
				FillRect(rb, Rect(1, 1) + Point(i, i) + fPos, 0xff000000);
			for (int i = -6; i <= 6; i++)
				FillRect(rb, Rect(1, 1) + Point(-i, i) + fPos, 0xff000000);
			
			for (int i = 0; i < 3; i++) {
				StrokeRect(rb, Rect(9, 9) + Point(8, 8) + Point(8*i, 0) + fPos, 1, 0xff000000);
				if ((1 << i) & fOldPointerBtns)
					FillRect(rb, Rect(9, 9) + Point(8, 8) + Point(8*i, 0) + fPos, 0xff000000);
			}
		}
	}

	void HandleMsg(Views::Message &_msg) override
	{
		if (auto msg = dynamic_cast<Views::FocusMsg*>(&_msg)) {
			if (msg->focus == Views::pointerFocus && !msg->on) {
				fFocus = false;
				Invalidate(Bounds());
			}
		} else if (auto msg = dynamic_cast<Views::PointerMsg*>(&_msg)) {
			if (!fFocus) {
				fFocus = true;
			}
			fPos = msg->pos;
			fOldPointerBtns = msg->btns;
			Invalidate(Bounds());
		}
	}
};

class DragView: public Views::View
{
private:
	enum Part {
		noPart,
		dragPart,
		resizePart
	};
	
	bool fPointerFocus, fTrack;
	Point fPos;
	uint32_t fBtns;
	Part fPart;
	

public:
	DragView(): fPointerFocus(false), fTrack(false) {}
	
	void Draw(RasBufOfs32 rb, Rect dirty) override
	{
		uint32_t c2 = 0xffff9900, c1 = 0xffffffff;
		FillRect(rb, Bounds(), c1);
		StrokeRect(rb, Bounds(), 2, c2);
		StrokeRect(rb, Rect(8, 8) + Bounds().RightBottom() - Point(8, 8), 2, c2);
	}

	void HandleMsg(Views::Message &_msg) override
	{
		if (auto msg = dynamic_cast<Views::FocusMsg*>(&_msg)) {
			if (msg->focus == Views::pointerFocus) {
				if (!msg->on) {
					fPointerFocus = false;
					fTrack = false;
					fBtns = false;
				}
			}
		} else if (auto msg = dynamic_cast<Views::PointerMsg*>(&_msg)) {
			if (!fPointerFocus) {
				fPointerFocus = true;
			} else {
				if (fBtns == 0) {
					fPos = msg->pos;
					fTrack = true;
					SetCapture(true);
					if ((Rect(8, 8) + Bounds().RightBottom() - Point(8, 8)).Contains(fPos))
						fPart = resizePart;
					else
						fPart = dragPart;
				} else if(fTrack && msg->btns == 0) {
					fTrack = false;
					SetCapture(false);
				}
				if (fTrack) {
					switch (fPart) {
					case dragPart:
						Up()->SetRect(this, Up()->GetRect(this) + (msg->pos - fPos));
						break;
					case resizePart: {
						Rect rect = Up()->GetRect(this);
						rect.r += (msg->pos - fPos).x;
						rect.b += (msg->pos - fPos).y;
						fPos = msg->pos;
						Up()->SetRect(this, rect);
						break;
					}
					default:
						break;
					}
				}
			}
			fBtns = msg->btns;
		}
	}

};

class AnimationView: public Views::View
{
private:
	int32_t fStep;
	
	class Timer: public Timers::Timer
	{
	public:
		void Do() override
		{
			Schedule(Timers::Time() + Timers::second/30);
			AnimationView *v = BasePtr(this, AnimationView, fTimer);
			v->fStep = (v->fStep + 1) % 128;
			if (v->fStep < 64)
				v->Up()->SetRect(v, v->Up()->GetRect(v) + Point(1, 0));
			else
				v->Up()->SetRect(v, v->Up()->GetRect(v) + Point(-1, 0));
			Views::Draw();
		}
	};
	friend class Timer;
	Timer fTimer;
	
public:
	AnimationView()
	{
		fTimer.Schedule(Timers::Time() + Timers::second/30);
	}

	~AnimationView()
	{
		fTimer.Cancel();
	}
	
	void Draw(RasBufOfs32 rb, Rect dirty) override
	{
		FillRect(rb, Bounds(), 0xff000000);
	}
};

class ShutdownAction: public Action
{
	void Do() override {Shutdown();}
};

class StackTraceAction: public Action
{
	void Do() override {Modules::StackTrace();}
};


void BuildViews()
{
	Views::View *rootView = new(std::nothrow) ColorView(0xffffffff);
	Views::gPort->SetRoot(rootView, gFrontBuf.width, gFrontBuf.height);
	Views::View *v2 = new(std::nothrow) TestView();
	rootView->Attach(Rect(192, 192) + Point(24, 32), v2);
	for (int i = 0; i < 24; i++)
		v2->Attach(Rect(64, 64) + Point(-16, 16) + Point(8*i, 8*i), new(std::nothrow) TestView());
	rootView->Attach(Rect(96, 96) + Point(128, 8), new(std::nothrow) PatternView());
	rootView->Attach(Rect(128, 128) + Point(256, 8), new(std::nothrow) PointerTestView());

	for (int i = 0; i < 4; i++)
		rootView->Attach(Rect(64, 64) + Point(8, 256) + Point(8*i, 8*i), new(std::nothrow) DragView());

	Views::View *v3 = new(std::nothrow) DragView();
	rootView->Attach(Rect(128, 96) + Point(128, 256), v3);
	v3->Attach(Rect(64, 64) + Point(8, 9), new(std::nothrow) DragView());

	for (int i = 0; i < 4; i++)
		rootView->Attach(Rect(16, 16) + Point(256 + 8, 256) + Point(0, i*(16 + 8)), new(std::nothrow) AnimationView());
	
	Point pos = Point(256 + 128 + 16, 16);
	rootView->Attach(
		Rect(64 + 16, 24) + pos,
		new(std::nothrow) Button("Shutdown", new(std::nothrow) ShutdownAction())
	);
	pos += Point(0, 32);
	rootView->Attach(
		Rect(64 + 16, 24) + pos,
		new(std::nothrow) Button("Stack trace", new(std::nothrow) StackTraceAction())
	);
}


//#pragma mark Entry point

extern "C" void Do(uint64_t cpuId, const void *fdt)
{
	WriteString("Do(");
	WriteInt(cpuId); WriteString(", 0x");
	WriteHex((size_t)fdt, 8);
	WriteString(")"); WriteLn();
/*
	for (int y = 0; y < gFrontBuf.height; y++) {
		for (int x = 0; x < gFrontBuf.width; x++) {
			gFrontBuf.colors[y*gFrontBuf.stride + x] = x%0x100 + y%0x100*0x100;
		}
	}
*/
	memset(startupBssBase, 0, startupBssEnd - startupBssBase);
	ProcessFdt(fdt);
	WriteString("Memory size: "); WriteInt(gMemorySize); WriteLn();
	AddCluster((void*)&startupEnd, gMemorySize - ((size_t)&startupEnd - memoryBase));

	DumpFdt(fdt);
	DumpPci();
/*
	{
		int node = -1, depth = -1;
		node = fdt_next_node(fdt, node, &depth);
		TraverseFdt(fdt, node, depth);
		Shutdown();
	}
*/
	Modules::Init();
	FwCfg::Init();
	Graphics::Init();

	Traps::Init();
	Plic::Init();
	Syscall(Traps::test1Syscall);
	Syscall(Traps::test2Syscall, 10, -15, 12345);
	Syscall(Traps::switchToSModeSyscall);
	
	// VM::Init();

	Views::Port port;
	Views::gPort = &port;
	BuildViews();
	Views::Draw();
/*
	ResetCaret();

	FillRect(gFrontBuf, Rect(32, 32, 800, 600), 0xff99ccff);
	StrokeRect(gFrontBuf, Rect(128, 192) + Point(512, 96), 3, 0xff666666);
	
	DrawString(gFrontBuf, gCarPos, 0xffff0000, "Red", gFont);
	DrawString(gFrontBuf, gCarPos, 0xff00ff00, " Green", gFont);
	DrawString(gFrontBuf, gCarPos, 0xff0000ff, " Blue", gFont); WriteLn();
	DrawString(gFrontBuf, gCarPos, 0xff0099ff, "This is a test.", gFont);
	DrawString(gFrontBuf, gCarPos, 0xffff0000, " 123.", gFont); WriteLn();
	WriteInt(-12345); WriteLn();
	WriteHex(0x123456789abcdef0, 16); WriteLn();
	WriteString("cpuId: "); WriteHex(cpuId, 16); WriteLn();
	WriteString("fdt: "); WriteHex((uint64_t)fdt, 16); WriteLn();
	WriteString("FP: "); WriteHex(Fp(), 8); WriteLn();
	WriteString("RA: "); WriteHex(Ra(), 8); WriteLn();
	WriteString("PC*: "); WriteHex(*((size_t*)Fp() - 1), 8); WriteLn();
	WriteString("FP*: "); WriteHex(*((size_t*)Fp() - 2), 8); WriteLn();
	
	int32_t val = 1 << 0, val2;
	val2 = atomic_or(&val, 1 << 1);
	WriteString("val2: "); WriteSet(val2); WriteLn();
		
	//AllocTest();
	//AllocTest();
	WriteLn();
*/

#if 0
	int64_t time = Timers::Time();
	(new TestTimer())->Schedule(time + 1*Timers::second/2);
	(new TestTimer())->Schedule(time + 2*Timers::second/2);
	(new TestTimer())->Schedule(time + 2*Timers::second/2);
	(new TestTimer())->Schedule(time + 5*Timers::second/2);
	(new TestTimer())->Schedule(time + 4*Timers::second/2);
	(new TestTimer())->Schedule(time + 3*Timers::second/2);
	
	Timers::Dump();
	WriteLn();
#endif


	Threads::Thread *startupTh = new Threads::Thread("startup", NULL, 0);
	//Thread *idleTh = new Thread("idle", 32*1024, IdleThreadEntry, NULL);
	
/*
	for (int i = 0; i < 6; i++) {
		WriteString("Virtio "); WriteInt(i); WriteLn();
		volatile VirtioRegs *d = (VirtioRegs*)((uint8_t*)gVirtioBase + i*virtioRegsSize);
		WriteString("signature: "); WriteHex(d->signature, 8); WriteLn();
		WriteString("version: "); WriteInt(d->version); WriteLn();
		WriteString("deviceId: "); WriteInt(d->deviceId); WriteLn();
		WriteString("vendorId: "); WriteHex(d->vendorId, 8); WriteLn();
	}
*/
	Virtio::List();
	Virtio::TestInput();
	
	gCarX0 = 256 + 128; gCarPos.x = gCarX0;
	gCarPos.y = 64;

#if 0
	TestThread *t1 = new TestThread("thread1", 256, 32 + (24)*0, 1000);
	TestThread *t2 = new TestThread("thread2", 256, 32 + (24)*1, 2000);
	TestThread *t3 = new TestThread("thread3", 256, 32 + (24)*2, 3000);
	TestThread *t4 = new TestThread("thread4", 256, 32 + (24)*3, 3000);
	TestThread *t5 = new TestThread("thread5", 256, 32 + (24)*4, 3000);
	TestThread *t6 = new TestThread("thread6", 256, 32 + (24)*5, 3000);

	WriteLn();
	Threads::Dump();
	
	int32_t res;
	res = Threads::Join(t1->ThisThread());
	WriteLn();
	WriteString("ThreadJoin(t1): "); WriteInt(res); WriteLn();
	Threads::Dump();

	res = Threads::Join(t2->ThisThread());
	WriteLn();
	WriteString("ThreadJoin(t2): "); WriteInt(res); WriteLn();
	Threads::Dump();

	res = Threads::Join(t3->ThisThread());
	WriteLn();
	WriteString("ThreadJoin(t3): "); WriteInt(res); WriteLn();
	Threads::Dump();

#endif
	for (;;) Wfi();
	for (;;) Threads::Yield();
}


extern "C" void abort()
{
		// DisableIntr();
		WriteTab(); WriteString("=== PANIC: abort ==="); WriteLn();
		Modules::StackTrace();
		// for(;;) {}
		Shutdown();
}

extern "C" int __cxa_atexit(void (*)(void*), void*, void*)
{
	return 0;
}
