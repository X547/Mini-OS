#include "Locks.h"
#include "RiscV.h"

namespace Locks {


extern "C" uint32_t atomic_or(uint32_t *dst, uint32_t src)
{
	return __atomic_fetch_or(dst, src, __ATOMIC_SEQ_CST);
}

extern "C" uint32_t atomic_and(uint32_t *dst, uint32_t src)
{
	return __atomic_fetch_and(dst, src, __ATOMIC_SEQ_CST);
}

extern "C" uint32_t atomic_get(uint32_t *src)
{
	return __atomic_load_n(src, __ATOMIC_SEQ_CST);
}


InterruptsLock::InterruptsLock(bool doLock): fLocked(false)
{
	fOldEnabled = IsIntrEnabled();
	if (doLock) Acquire();
}

InterruptsLock::~InterruptsLock()
{
	Release();
}

void InterruptsLock::Acquire()
{
	if (fOldEnabled && !fLocked) {
		DisableIntr();
		fLocked = true;
	}
}

void InterruptsLock::Release()
{
	if (fOldEnabled && fLocked) {
		fLocked = false;
		EnableIntr();
	}
}


Spinlock::Spinlock(): fValue(0)
{}

void Spinlock::Acquire()
{
	fOldIntsEnabled = DisableIntr();
  while(__sync_lock_test_and_set(&fValue, 1) != 0) {}
  __sync_synchronize();
}

void Spinlock::Release()
{
  __sync_synchronize();
  __sync_lock_release(&fValue);
  RestoreIntr(fOldIntsEnabled);
}


void WaitList::Acquire()
{
	thread = Threads::Current();
	waiting = 1;
	while ((atomic_get(&waiting)) != 0)
		Threads::Wait();
}

void WaitList::Release()
{
	waiting = 0;
	Threads::Wake(thread, true);
}

Mutex::Mutex(bool acquireFirst):
	fValue(acquireFirst? State::acquired: 0)
{
	fWaitList = {&fWaitList, &fWaitList};
}

bool Mutex::Acquire()
{
	uint32_t oldValue;

	oldValue = atomic_or(&fValue, State::acquired);
	if ((oldValue & (State::acquired | State::waiting)) == 0)
		return true;

	WaitList wait;
	Spinlock spinlock;
	spinlock.Acquire();
	fWaitList.InsertBefore(&wait);
	spinlock.Release();
	
	wait.Acquire();

	//atomic_or(&fValue, State::waiting);
	while ((atomic_get(&wait.waiting)) != 0)
		Threads::Wait();

	return true;
}

void Mutex::Release(bool releaseAll)
{
	uint32_t oldValue = atomic_and(&fValue, ~(uint32_t)State::acquired);
	if ((oldValue & State::waiting) == 0)
		return;

	Spinlock spinlock;
	spinlock.Acquire();
	WaitList *wait = (WaitList*)fWaitList.fNext;
	fWaitList.Remove(wait);
	spinlock.Release();
	wait->Release();
	atomic_or(&fValue, State::acquired);
	atomic_and(&fValue, ~State::waiting);
	Threads::Wake(wait->thread, true);
}

void ReleaseIfWaiting(bool releaseAll)
{
}

bool SwitchFrom(Mutex &src)
{
	return false;
}

}
