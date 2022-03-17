#ifndef _DEVICES_H_
#define _DEVICES_H_

namespace Devices {

class Id {
};

class FdtId: public Id {
public:
	const char *Compatible();
};

enum {
	mmioResource,
	irqResource,
} ResourceKind;

class Resource {
public:
	void Acquire();
	void Release();
};

class MmioResource: public Resource {
public:
	void *VirtAdr();
	size_t PhysAdr();
	size_t Size();
};

class IrqResoruce: public Resource {
public:
	virtual void SetHandler(void (*Handler)(void *arg), void *arg) = 0;
};

struct ResourceGroup {
	Resource **resource;
	size_t count;
};

class Device {
private:
	ResourceGroup fResources[2];
	
public:
	Resoruce *ThisResource(ResourceKind kind, uint32 ord);
	void AddResource();
};

}

#endif	// _DEVICES_H_
