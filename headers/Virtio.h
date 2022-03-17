#ifndef _VIRTIO_H_
#define _VIRTIO_H_

#include <stddef.h>
#include <stdint.h>
#include "AutoDeleter.h"

namespace Virtio {

enum {
	virtioRegsSize = 0x1000,
	virtioSignature = 0x74726976,
	virtioVendorId = 0x554d4551,
};

enum {
	virtioDevNet = 1,
	virtioDevBlock = 2,
	virtioDevConsole = 3,
	virtioDev9p = 9,
	virtioDevInput = 18,
};

enum {
	virtioConfigSAcknowledge = 1 << 0,
	virtioConfigSDriver      = 1 << 1,
	virtioConfigSDriverOk    = 1 << 2,
	virtioConfigSFeaturesOk  = 1 << 3,
};

// VirtioRegs.interruptStatus, interruptAck
enum {
	virtioIntQueue  = 1 << 0,
	virtioIntConfig = 1 << 1,
};

enum {
	vringDescFlagsNext     = 1 << 0,
	vringDescFlagsWrite    = 1 << 1,
	vringDescFlagsIndirect = 1 << 2,
};

struct VirtioRegs {
	uint32_t signature;
	uint32_t version;
	uint32_t deviceId;
	uint32_t vendorId;
	uint32_t deviceFeatures;
	uint32_t unknown1[3];
	uint32_t driverFeatures;
	uint32_t unknown2[1];
	uint32_t guestPageSize; /* version 1 only */
	uint32_t unknown3[1];
	uint32_t queueSel;
	uint32_t queueNumMax;
	uint32_t queueNum;
	uint32_t queueAlign;    /* version 1 only */
	uint32_t queuePfn;      /* version 1 only */
	uint32_t queueReady;
	uint32_t unknown4[2];
	uint32_t queueNotify;
	uint32_t unknown5[3];
	uint32_t interruptStatus;
	uint32_t interruptAck;
	uint32_t unknown6[2];
	uint32_t status;
	uint32_t unknown7[3];
	uint32_t queueDescLow;
	uint32_t queueDescHi;
	uint32_t unknown8[2];
	uint32_t queueAvailLow;
	uint32_t queueAvailHi;
	uint32_t unknown9[2];
	uint32_t queueUsedLow;
	uint32_t queueUsedHi;
	uint32_t unknown10[21];
	uint32_t configGeneration;
	uint8_t config[3840];
};

struct VirtioDesc {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
};
// filled by driver
struct VirtioAvail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[0];
};
struct VirtioUsedItem
{
	uint32_t id;
	uint32_t len;
};
// filled by device
struct VirtioUsed {
	uint16_t flags;
	uint16_t idx;
	VirtioUsedItem ring[0];
};


// Input

// VirtioInputConfig::select
enum {
	virtioInputCfgUnset    = 0x00,
	virtioInputCfgIdName   = 0x01,
	virtioInputCfgIdSerial = 0x02,
	virtioInputCfgIdDevids = 0x03,
	virtioInputCfgPropBits = 0x10,
	virtioInputCfgEvBits   = 0x11, // subsel: virtioInputEv*
	virtioInputCfgAbsInfo  = 0x12, // subsel: virtioInputAbs*
};

enum {
	virtioInputEvSyn = 0,
	virtioInputEvKey = 1,
	virtioInputEvRel = 2,
	virtioInputEvAbs = 3,
	virtioInputEvRep = 4,
};

enum {
	virtioInputBtnLeft     = 0x110,
	virtioInputBtnRight    = 0x111,
	virtioInputBtnMiddle   = 0x112,
	virtioInputBtnGearDown = 0x150,
	virtioInputBtnGearUp   = 0x151,
};

enum {
	virtioInputRelX     = 0,
	virtioInputRelY     = 1,
	virtioInputRelZ     = 2,
	virtioInputRelWheel = 8,
};

enum {
	virtioInputAbsX = 0,
	virtioInputAbsY = 1,
	virtioInputAbsZ = 2,
};


struct VirtioInputAbsinfo {
	int32_t min;
	int32_t max;
	int32_t fuzz;
	int32_t flat;
	int32_t res;
};

struct VirtioInputDevids {
	uint16_t bustype;
	uint16_t vendor;
	uint16_t product;
	uint16_t version;
};

struct VirtioInputConfig {
	uint8_t select; // in
	uint8_t subsel; // in
	uint8_t size;   // out, size of reply
	uint8_t reserved[5];
	union {
		char string[128];
		uint8_t bitmap[128];
		VirtioInputAbsinfo abs;
		VirtioInputDevids ids;
	};
};

struct VirtioInputPacket {
	uint16_t type;
	uint16_t code;
	int32_t value;
};


extern uint32_t *gVirtioBase;


enum IOState {
	ioStateInactive,
	ioStatePending,
	ioStateDone,
	ioStateFailed,
};

enum IOOperation {
	ioOpRead,
	ioOpWrite,
};

struct IORequest {
	IOState state;
	IOOperation op;
	void *buf;
	size_t len;
	IORequest *next;

	IORequest(IOOperation op, void *buf, size_t len): state(ioStateInactive), op(op), buf(buf), len(len), next(NULL) {}
};

struct Queue {
	size_t fQueueLen;
	VirtioDesc *volatile fDescs;
	VirtioAvail *volatile fAvail;
	VirtioUsed *volatile fUsed;
	ArrayDeleter<uint32_t> fFreeDescs;
	uint32_t fLastUsed;
	ArrayDeleter<IORequest*> fReqs;

	int32_t AllocDesc();
	void FreeDesc(int32_t idx);

	Queue(int32_t id, VirtioRegs *volatile fRegs);
	~Queue();
};

struct VirtioDev
{
	VirtioRegs *volatile fRegs;
	int32_t fIrq;
	int32_t fQueueCnt;
	Queue **fQueues;

	void Init(int32_t queueCnt);
	void ScheduleIO(int32_t queueId, IORequest **reqs, uint32_t cnt);
	void ScheduleIO(int32_t queueId, IORequest *req);
	IORequest *ConsumeIO(int32_t queueId);

};


void Register(void *regs, size_t regsLen, int32_t irq);
void List();

void TestInput();
void IrqHandler(uint32_t irq);

}

#endif	// _VIRTIO_H_
