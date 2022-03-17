#ifndef _TRAPS_H_
#define _TRAPS_H_

#include <stdint.h>


extern "C" void MVec();
extern "C" void SVec();

namespace Traps {

struct TrapFrame {
	uint64_t r[32];
};


enum {
	test1Syscall,
	test2Syscall,
	switchToSModeSyscall,
	enableTimerSyscall,
};

int64_t Syscall(...);

extern "C" void MTrap(Traps::TrapFrame &tf);
extern "C" void STrap(Traps::TrapFrame &tf);

void Init();

}


#endif	// _TRAPS_H_
