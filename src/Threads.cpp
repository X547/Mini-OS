#include "Threads.h"
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include "RiscV.h"
#include "Timers.h"
#include "Graphics.h"
#include "Modules.h"

namespace Threads {

enum {
	preemptTimeout = Timers::second/500,
};

Lists::List gReadyThreads = {&gReadyThreads, &gReadyThreads};
Lists::List gWaitingThreads = {&gWaitingThreads, &gWaitingThreads};


static inline void SetCurrent(Thread *th) {asm volatile("mv tp, %0" : : "r" (th));}


class PreemptTimer: public Timers::Timer
{
public:
	void Do()
	{
		Threads::Yield();
	}
};

PreemptTimer *gPreemptTimer = NULL;


[[noreturn]] void __attribute__((naked)) ThreadEntry()
{
	// enable interrupts
	__asm("csrr a0, mstatus");
	__asm("ori a0, a0, 1 << 3");
	__asm("csrw mstatus, a0");
/*
	__asm("LI ra, 0");
	__asm("ADDI sp, sp, -16");
	__asm("SD ra, 8(sp)");
	__asm("SD fp, 0(sp)");
	__asm("ADDI fp, sp, 16");
*/
	__asm("MV a0, s2");
	__asm("JALR s1");
	__asm("CALL _ZN7Threads4ExitEi");
}


Thread::Thread(const char *name, size_t stackSize, int32_t (*Entry)(void *arg), void *arg):
	fState(State::inactive), fJoin(NULL)
{
	strlcpy(fName, name, 32);
	fStack = aligned_malloc(stackSize, 16);
	fStackSize = stackSize;

	fContext[0].ra = (size_t)ThreadEntry;
	fContext[0].s[0] = 0;
	fContext[0].s[1] = (size_t)Entry;
	fContext[0].s[2] = (size_t)arg;
	fContext[0].sp = (size_t)fStack + fStackSize;

	fState = State::ready;
	gReadyThreads.InsertBefore(this);
}

Thread::Thread(const char *name, void *stack, size_t stackSize):
	fState(State::inactive), fJoin(NULL)
{
	strlcpy(fName, name, 32);
	fStack = stack;
	fStackSize = stackSize;
	fState = State::running;
	SetCurrent(this);
	
	gPreemptTimer = new PreemptTimer();
	gPreemptTimer->Schedule(Timers::Time() + preemptTimeout);
	WriteString("gPreemptTimer: "); WriteHex((size_t)gPreemptTimer, 8); WriteLn();
}

Thread::~Thread()
{
	free(fStack);
}


void Switch(Thread *th, State state)
{
	// Modules::StackTrace();
	if (th != Current()) {
		switch (state) {
		case State::ready:   gReadyThreads.  InsertBefore(Current()); break;
		case State::waiting: gWaitingThreads.InsertBefore(Current()); break;
		case State::done: break;
		default: return;
		}
		Current()->fState = state;
		if (setjmp(Current()->fContext) == 0) {
/*
			WriteString("ThreadSwitch("); WriteString(Current()->Name()); WriteString(", ");
			WriteHex(Current()->fContext[0].ra, 8); WriteString(", ");
			WriteString(th->Name()); WriteString(", ");
			WriteHex(th->fContext[0].ra, 8); WriteString(")"); WriteLn();
*/
			SetCurrent(th);
			th->fState = State::running;
			gReadyThreads.Remove(th);
/*
			WriteString("longjmp"); WriteLn();
*/
			longjmp(th->fContext, 1);
		} else {
/*
			WriteString("EnableInts"); WriteLn();
*/
			EnableIntr();
		}
	}
}

static Thread *Next()
{
	Thread *next = (Thread*)gReadyThreads.fNext;
	// no ready threads -> keep current thread running
	if (next == (Thread*)&gReadyThreads) return Current();
	gPreemptTimer->Schedule(Timers::Time() + preemptTimeout);
	return next;
}

void Yield()
{
	Switch(Next(), State::ready);
}

void Wait()
{
	Switch(Next(), State::waiting);
}

void Exit(int32_t res)
{
	Current()->fRes = res;
	if (Current()->fJoin != NULL)
		Wake(Current()->fJoin, false);
	Switch(Next(), State::done);
	__builtin_unreachable();
}

void Wake(Thread *th, bool doSwitch)
{
	if (th->fState == State::waiting) {
		gWaitingThreads.Remove(th);
		th->fState = State::ready;
		gReadyThreads.InsertBefore(th);
		if (doSwitch) Switch(th, State::ready);
	}
}

int32_t Join(Thread *th)
{
	if (th == Current() || th->fJoin != NULL)
		return -1;
	int32_t res;
	if (th->fState != State::done) {
		th->fJoin = Current();
		Wait();
	}
	res = th->fRes;
	delete th;
	return res;
}

void Dump()
{
	WriteString("curThread: "); if (Current() == NULL) WriteString("NULL"); else WriteString(Current()->Name()); WriteLn();
	WriteString("readyThreads:"); WriteLn();
	for (Thread *it = (Thread*)gReadyThreads.fNext; it != &gReadyThreads; it = (Thread*)it->fNext) {
		WriteTab(); WriteString(it->Name()); WriteLn();
	}
	WriteString("waitingThreads:"); WriteLn();
	for (Thread *it = (Thread*)gWaitingThreads.fNext; it != &gWaitingThreads; it = (Thread*)it->fNext) {
		WriteTab(); WriteString(it->Name()); WriteLn();
	}
}

}
