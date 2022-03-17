#include "Virtio.h"
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <new>

#include "Plic.h"
#include "Graphics.h"
#include "Modules.h"
#include "Views.h"


namespace Virtio {

struct VirtioResources {
	VirtioRegs *volatile regs;
	size_t regsLen;
	int32_t irq;
};

VirtioResources gVirtioDevList[32];
int32_t gVirtioDevListLen = 0;


Queue::Queue(int32_t id, VirtioRegs *volatile fRegs)
{
	fRegs->queueSel = id;
//	WriteString("queueNumMax: "); WriteInt(fRegs->queueNumMax); WriteLn();
	fQueueLen = fRegs->queueNumMax;
	fRegs->queueNum = fQueueLen;
	fLastUsed = 0;
	
	fDescs = (VirtioDesc*)aligned_malloc(sizeof(VirtioDesc)*fQueueLen, 4096);
	memset(fDescs, 0, sizeof(VirtioDesc)*fQueueLen);
	fAvail = (VirtioAvail*)aligned_malloc(sizeof(VirtioAvail) + sizeof(uint16_t)*fQueueLen, 4096);
	memset(fAvail, 0, sizeof(VirtioAvail) + sizeof(uint16_t)*fQueueLen);
	fUsed = (VirtioUsed*)aligned_malloc(sizeof(VirtioUsed) + sizeof(VirtioUsedItem)*fQueueLen, 4096);
	memset(fUsed, 0, sizeof(VirtioUsed) + sizeof(VirtioUsedItem)*fQueueLen);
	fFreeDescs.SetTo(new(std::nothrow) uint32_t[(fQueueLen + 31)/32]);
	memset(fFreeDescs.Get(), 0xff, sizeof(uint32_t)*((fQueueLen + 31)/32));
	
	fRegs->queueDescLow = (uint32_t)(uint64_t)fDescs;
	fRegs->queueDescHi = (uint32_t)((uint64_t)fDescs >> 32);
	fRegs->queueAvailLow = (uint32_t)(uint64_t)fAvail;
	fRegs->queueAvailHi = (uint32_t)((uint64_t)fAvail >> 32);
	fRegs->queueUsedLow = (uint32_t)(uint64_t)fUsed;
	fRegs->queueUsedHi = (uint32_t)((uint64_t)fUsed >> 32);
/*
	WriteString("fDescs: "); WriteHex((uint64_t)fDescs, 8); WriteLn();
	WriteString("fAvail: "); WriteHex((uint64_t)fAvail, 8); WriteLn();
	WriteString("fUsed: "); WriteHex((uint64_t)fUsed, 8); WriteLn();
*/
	
	fReqs.SetTo(new(std::nothrow) IORequest*[fQueueLen]);
	
	fRegs->queueReady = 1;
}

Queue::~Queue()
{
	aligned_free(fDescs); fDescs = NULL;
	aligned_free(fAvail); fAvail = NULL;
	aligned_free(fUsed); fUsed = NULL;
}


int32_t Queue::AllocDesc()
{
	for (int i = 0; i < fQueueLen; i++) {
		if ((fFreeDescs[i/32] & (1 << (i % 32))) != 0) {
			fFreeDescs[i/32] &= ~((uint32_t)1 << (i % 32));
			return i;
		}
	}
	return -1;
}

void Queue::FreeDesc(int32_t idx)
{
	fFreeDescs[idx/32] |= (uint32_t)1 << (idx % 32);
}

void VirtioDev::Init(int32_t queueCnt)
{
	WriteString("0x"); WriteHex((size_t)fRegs, 8); WriteString(".version: "); WriteInt(fRegs->version); WriteLn();
	fRegs->status |= virtioConfigSAcknowledge;
	fRegs->status |= virtioConfigSDriver;
	WriteString("0x"); WriteHex((size_t)fRegs, 8); WriteString(".features: "); WriteHex(fRegs->deviceFeatures, 8); WriteLn();
	fRegs->status |= virtioConfigSFeaturesOk;
	fRegs->status |= virtioConfigSDriverOk;
	Plic::EnableIrq(fIrq, true);

	fQueueCnt = queueCnt;
	fQueues = new(std::nothrow) Queue*[fQueueCnt];
	for (int32_t i = 0; i < fQueueCnt; i++) {
		fQueues[i] = new(std::nothrow) Queue(i, fRegs);
	}
}

void VirtioDev::ScheduleIO(int32_t queueId, IORequest **reqs, uint32_t cnt)
{
	if (cnt < 1) return;
	if (!(queueId >= 0 && queueId < fQueueCnt)) abort();
	Queue *queue = fQueues[queueId];
	fRegs->queueSel = queueId;
	int32_t firstDesc, lastDesc;
	for (uint32_t i = 0; i < cnt; i++) {
		int32_t desc = queue->AllocDesc();
		if (desc < 0) {abort(); return;}
		if (i == 0) {
			firstDesc = desc;
		} else {
			queue->fDescs[lastDesc].flags |= vringDescFlagsNext;
			queue->fDescs[lastDesc].next = desc;
			reqs[i - 1]->next = reqs[i];
		}
		queue->fDescs[desc].addr = (uint64_t)(reqs[i]->buf);
		queue->fDescs[desc].len = reqs[i]->len;
		queue->fDescs[desc].flags = 0;
		queue->fDescs[desc].next = 0;
		switch (reqs[i]->op) {
		case ioOpRead: queue->fDescs[desc].flags |= vringDescFlagsWrite; break;
		case ioOpWrite: break;
		}
		reqs[i]->state = ioStatePending;
		lastDesc = desc;
	}
	int32_t idx = queue->fAvail->idx % queue->fQueueLen;
	queue->fReqs[idx] = reqs[0];
	queue->fAvail->ring[idx] = firstDesc;
	queue->fAvail->idx++;
	fRegs->queueNotify = queueId;
}

void VirtioDev::ScheduleIO(int32_t queueId, IORequest *req)
{
	ScheduleIO(queueId, &req, 1);
}

IORequest *VirtioDev::ConsumeIO(int32_t queueId)
{
	if (!(queueId >= 0 && queueId < fQueueCnt)) abort();
	Queue *queue = fQueues[queueId];
	fRegs->queueSel = queueId;
	if (queue->fUsed->idx == queue->fLastUsed) return NULL;
	IORequest *req = queue->fReqs[queue->fLastUsed % queue->fQueueLen]; queue->fReqs[queue->fLastUsed % queue->fQueueLen] = NULL;
	req->len = queue->fUsed->ring[queue->fLastUsed % queue->fQueueLen].len;
	req->state = ioStateDone;
	int32_t desc = queue->fUsed->ring[queue->fLastUsed % queue->fQueueLen].id;
	// WriteString("queue->fUsed->ring[queue->fLastUsed % queue->fQueueLen].len: "); WriteInt(queue->fUsed->ring[queue->fLastUsed % queue->fQueueLen].len); WriteLn();
	while (vringDescFlagsNext & queue->fDescs[desc].flags) {
		int32_t nextDesc = queue->fDescs[desc].next;
		queue->FreeDesc(desc);
		desc = nextDesc;
	}
	queue->FreeDesc(desc);
	queue->fLastUsed++;
	return req;
}


VirtioDev *gInputKeyboardDev = NULL;
VirtioDev *gInputTabletDev = NULL;
VirtioDev *gConsoleDev = NULL;


VirtioRegs *volatile ThisVirtioDev(uint32_t deviceId, int n)
{
	for (int i = 0; i < gVirtioDevListLen; i++) {
		VirtioRegs *volatile regs = gVirtioDevList[i].regs;
		if (regs->signature != virtioSignature) continue;
		if (regs->deviceId == deviceId) {if (n == 0) return regs; else n--;}
	}
	return NULL;
}

int32_t ThisVirtioDev2(uint32_t deviceId, int n)
{
	for (int i = 0; i < gVirtioDevListLen; i++) {
		VirtioRegs *volatile regs = gVirtioDevList[i].regs;
		if (regs->signature != virtioSignature) continue;
		if (regs->deviceId == deviceId) {if (n == 0) return i; else n--;}
	}
	return -1;
}


void Register(void *regs, size_t regsLen, int32_t irq)
{
	gVirtioDevList[gVirtioDevListLen].regs = (VirtioRegs *volatile)regs;
	gVirtioDevList[gVirtioDevListLen].regsLen = regsLen;
	gVirtioDevList[gVirtioDevListLen].irq = irq;
	gVirtioDevListLen++;
}

void List()
{
	WriteString("VirtIO devices:"); WriteLn();
	for (int i = 0; i < gVirtioDevListLen; i++) {
		VirtioRegs *volatile d = gVirtioDevList[i].regs;
		if (d->signature != virtioSignature) continue;
		WriteTab(); WriteInt(i);
		WriteString(": (0x"); WriteHex((size_t)gVirtioDevList[i].regs, 0);
		WriteString(", 0x"); WriteHex((size_t)gVirtioDevList[i].regsLen, 0);
		WriteString(", "); WriteInt(gVirtioDevList[i].irq);
		WriteString("): ");
		uint32_t deviceId = d->deviceId;
		switch (deviceId) {
		case virtioDevNet:     WriteString("net"); break;
		case virtioDevBlock:   WriteString("block"); break;
		case virtioDevConsole: WriteString("console"); break;
		case virtioDev9p:      WriteString("9p"); break;
		case virtioDevInput:
			WriteString("input(name: \"");
			d->config[0] = virtioInputCfgIdName;
			WriteString((const char*)(&d->config[8]));
			WriteString("\")");
			break;
		default:
			WriteString("("); WriteInt(deviceId); WriteString(")"); break;
		}
		WriteLn();
	}
}


void TestInput()
{
	int32_t inputKeyboardDev = ThisVirtioDev2(virtioDevInput, 0);
	if (inputKeyboardDev == -1) {WriteString("virtio input keyboard device not found"); WriteLn(); return;}
	gInputKeyboardDev = new (std::nothrow) VirtioDev();
	gInputKeyboardDev->fRegs = gVirtioDevList[inputKeyboardDev].regs;
	gInputKeyboardDev->fIrq = gVirtioDevList[inputKeyboardDev].irq;
	gInputKeyboardDev->Init(1);
	for (int i = 0; i < 4; i++)
		gInputKeyboardDev->ScheduleIO(0, new(std::nothrow) IORequest(ioOpRead, malloc(sizeof(VirtioInputPacket)), sizeof(VirtioInputPacket)));

	int32_t inputTabletDev = ThisVirtioDev2(virtioDevInput, 1);
	if (inputTabletDev == -1) {WriteString("virtio input tablet device not found"); WriteLn(); return;}
	gInputTabletDev = new (std::nothrow) VirtioDev();
	gInputTabletDev->fRegs = gVirtioDevList[inputTabletDev].regs;
	gInputTabletDev->fIrq = gVirtioDevList[inputTabletDev].irq;
	gInputTabletDev->Init(1);
	SetCursor(&gPointer);
	for (int i = 0; i < 8; i++)
		gInputTabletDev->ScheduleIO(0, new(std::nothrow) IORequest(ioOpRead, malloc(sizeof(VirtioInputPacket)), sizeof(VirtioInputPacket)));

	int32_t consoleDevIdx = ThisVirtioDev2(virtioDevConsole, 0);
	if (consoleDevIdx == -1) {WriteString("virtio console not found"); WriteLn(); return;}
	gConsoleDev = new (std::nothrow) VirtioDev();
	gConsoleDev->fRegs = gVirtioDevList[consoleDevIdx].regs;
	gConsoleDev->fIrq = gVirtioDevList[consoleDevIdx].irq;
	gConsoleDev->Init(2);
	const char *str = "This is a virtio console test.\n";
	for (int i = 0; i < 4; i++)
		gConsoleDev->ScheduleIO(1, new(std::nothrow) IORequest(ioOpWrite, (void*)str, strlen(str)));
	for (int i = 0; i < 2; i++)
		gConsoleDev->ScheduleIO(0, new(std::nothrow) IORequest(ioOpRead, malloc(32), 32));
}

void WriteInputPacket(const VirtioInputPacket &pkt)
{
	switch (pkt.type) {
		case virtioInputEvSyn: WriteString("syn"); break;
		case virtioInputEvKey: WriteString("key");
			WriteString(", ");
			switch (pkt.code) {
				case virtioInputBtnLeft:     WriteString("left");     break;
				case virtioInputBtnRight:    WriteString("middle");   break;
				case virtioInputBtnMiddle:   WriteString("right");    break;
				case virtioInputBtnGearDown: WriteString("gearDown"); break;
				case virtioInputBtnGearUp:   WriteString("gearUp");   break;
				default: WriteInt(pkt.code);
			}
			break;
		case virtioInputEvRel: WriteString("rel");
			WriteString(", ");
			switch (pkt.code) {
				case virtioInputRelX:     WriteString("relX");     break;
				case virtioInputRelY:     WriteString("relY");     break;
				case virtioInputRelZ:     WriteString("relZ");     break;
				case virtioInputRelWheel: WriteString("relWheel"); break;
				default: WriteInt(pkt.code);
			}
			break;
		case virtioInputEvAbs: WriteString("abs");
			WriteString(", ");
			switch (pkt.code) {
				case virtioInputAbsX: WriteString("absX"); break;
				case virtioInputAbsY: WriteString("absY"); break;
				case virtioInputAbsZ: WriteString("absZ"); break;
				default: WriteInt(pkt.code);
			}
			break;
		case virtioInputEvRep: WriteString("rep"); break;
	}
	switch (pkt.type) {
		case virtioInputEvSyn: break;
		case virtioInputEvKey: WriteString(", "); if (pkt.value == 0) WriteString("up"); else if (pkt.value == 1) WriteString("down"); else WriteInt(pkt.value); break;
		default: WriteString(", "); WriteInt(pkt.value);
	}
	WriteLn();
}

void ConsoleIrqHandler(VirtioDev *dev)
{
	dev->fRegs->interruptAck = dev->fRegs->interruptStatus & (virtioIntQueue | virtioIntConfig);
	IORequest *req = dev->ConsumeIO(1);
	if (req != NULL) {
		delete req;
	}
	req = dev->ConsumeIO(0);
	if (req != NULL) {
		WriteString((char*)req->buf, req->len); WriteLn();
		free(req->buf); req->buf = NULL; delete req;
		dev->ScheduleIO(0, new(std::nothrow) IORequest(ioOpRead, malloc(32), 32));
	}
}

void KeyboardIrqHandler(VirtioDev *dev)
{
	dev->fRegs->interruptAck = dev->fRegs->interruptStatus & (virtioIntQueue | virtioIntConfig);
	while (IORequest *req = dev->ConsumeIO(0)) {
		VirtioInputPacket &pkt = *(VirtioInputPacket*)req->buf;
		// WriteString("keyboard: "); WriteInputPacket(pkt);
		if (pkt.type == virtioInputEvKey) {
			if (pkt.code == 94 /* space */ && pkt.value == 1)
				Clear(gFrontBuf, 0xffdddddd);
			if (pkt.code == 18 /* 1 */ && pkt.value == 1) {
				WriteString("Heap memory: "); WriteInt(AllocatedMem()); WriteString("/"); WriteInt(TotalMem());
				WriteString(" ("); WriteInt(AllocatedMem()*100/TotalMem()); WriteString("%)"); WriteLn();
			}

			if (Views::gPort != NULL) {
				Views::KeyMsg msg(pkt.code, pkt.value != 0);
				Views::gPort->HandleMsg(msg);
				Views::Draw();
			}
		}

		free(req->buf); req->buf = NULL; delete req;
		dev->ScheduleIO(0, new(std::nothrow) IORequest(ioOpRead, malloc(sizeof(VirtioInputPacket)), sizeof(VirtioInputPacket)));
	}
}

void TabletIrqHandler(VirtioDev *dev)
{
	dev->fRegs->interruptAck = dev->fRegs->interruptStatus & (virtioIntQueue | virtioIntConfig);
	int32_t x, y;
	uint32_t btns = gCursorBtns;
	while (IORequest *req = dev->ConsumeIO(0)) {
		VirtioInputPacket &pkt = *(VirtioInputPacket*)req->buf;
		switch (pkt.type) {
			case virtioInputEvSyn: {
				MoveCursor(gFrontBuf.width*x/32768, gFrontBuf.height*y/32768);
				gCursorBtns = btns;
				if (Views::gPort != NULL) {
					Views::PointerMsg msg(gCursorPos, gCursorBtns);
					Views::gPort->HandleMsg(msg);
					Views::Draw();
				}
				// if (gCursorPos.x < 512) SetCursor(&gPointer); else SetCursor(&gIBeam);
				break;
			}
			case virtioInputEvAbs:
				switch (pkt.code) {
					case virtioInputAbsX: x = pkt.value; break;
					case virtioInputAbsY: y = pkt.value; break;
				}
				break;
			case virtioInputEvKey:
				int32_t btn = -1;
				switch (pkt.code) {
				case virtioInputBtnLeft:   btn = 0; break;
				case virtioInputBtnRight:  btn = 1; break;
				case virtioInputBtnMiddle: btn = 2; break;
				}
				if (btn != -1) {
					if (pkt.value != 0)
						btns |= 1 << btn;
					else 
						btns &= ~(1 << btn);
				}
				break;
		}
		// WriteString("tablet: "); WriteInputPacket(pkt);
		free(req->buf); req->buf = NULL; delete req;
		dev->ScheduleIO(0, new(std::nothrow) IORequest(ioOpRead, malloc(sizeof(VirtioInputPacket)), sizeof(VirtioInputPacket)));
	}
}

void IrqHandler(uint32_t irq)
{
	// WriteString("IrqHandler("); WriteInt(irq); WriteString(")"); WriteLn();
	if (gConsoleDev != NULL && irq == gConsoleDev->fIrq) {
		ConsoleIrqHandler(gConsoleDev);
	} else if (gInputKeyboardDev != NULL && irq == gInputKeyboardDev->fIrq) {
		KeyboardIrqHandler(gInputKeyboardDev);
	} else if (gInputTabletDev != NULL && irq == gInputTabletDev->fIrq) {
		TabletIrqHandler(gInputTabletDev);
	}
}

}
