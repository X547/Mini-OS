#include "Timers.h"
#include "RiscV.h"
#include "Traps.h"
#include "Graphics.h"
#include <stddef.h>

namespace Timers {

volatile ClintRegs *gClintRegs;

Lists::List gTimers = {&gTimers, &gTimers};
bool gTimerEnabled = false;


static void ScheduleTimer(int64_t time)
{
	gClintRegs->mTimeCmp[0] = (uint64_t)time;
}

static int64_t ScheduledTime()
{
	return (int64_t)gClintRegs->mTimeCmp[0];
}

static void EnableTimer(bool enable)
{
	gTimerEnabled = enable;
	Syscall(Traps::enableTimerSyscall, enable);
/*
	if (enable)
		SetMie(Mie() | (1 << 7));
	else
		SetMie(Mie() & ~(1 << 7));
*/
}

static bool TimerEnabled()
{
	// return (Mie() & (1 << 7)) != 0;
	return gTimerEnabled;
}


Timer::Timer(): fScheduled(false)
{}

Timer::~Timer()
{
	Cancel();
}

void Timer::Schedule(int64_t time)
{
	if (fScheduled) Cancel();
		
	Timer *it = (Timer*)gTimers.fNext;
	while (it != &gTimers && !(time - it->fTime < 0))
		it = (Timer*)it->fNext;
	it->InsertBefore(this);
	fScheduled = true;

	fTime = time;
	if (!TimerEnabled()) {
		ScheduleTimer(fTime);
		EnableTimer(true);
	} else if (fTime - ScheduledTime() < 0)
		ScheduleTimer(fTime);
}

void Timer::Cancel()
{
	if (fScheduled) {
		fScheduled = false;
		gTimers.Remove(this);
	}
}


void Trap()
{
	int64_t time = Time();

	EnableTimer(false);

	for (;;) {
		Timer *it = (Timer*)gTimers.fNext;
		if (it == &gTimers) break;
		if (time - it->fTime < 0) {
			EnableTimer(true);
			ScheduleTimer(it->fTime);
			break;
		}
		it->Cancel();
		it->Do();
	}
}


void Dump()
{
	WriteString("Timers:"); WriteLn();
	for (Timer *it = (Timer*)gTimers.fNext; it != &gTimers; it = (Timer*)it->fNext) {
		WriteHex((size_t)it, 8); WriteString(": "); WriteInt(it->fTime); WriteLn();
	}
}

}
