#ifndef _TIMERS_H_
#define _TIMERS_H_

#include <stdint.h>
#include "Lists.h"

namespace Timers {

enum {
	second = 10000000 // unit of ClintRegs::mTime
};

// core local interruptor
struct ClintRegs
{
	uint8_t unknown1[0x4000];
	uint64_t mTimeCmp[4095]; // per CPU core, but not implemented in temu
	uint64_t mTime;          // @0xBFF8
};

volatile extern ClintRegs *gClintRegs;


class Timer: private Lists::List
{
private:
	int64_t fTime;
	bool fScheduled;

	friend void Trap();
	friend void Dump();
	
public:
	Timer();
	virtual ~Timer();

	void Schedule(int64_t time);
	void Cancel();

	// executed from trap handler with disabled interrupts
	virtual void Do() = 0;
};

static inline int64_t Time() {return (int64_t)gClintRegs->mTime;}

void Trap();

void Dump();

}

#endif	// _TIMERS_H_
