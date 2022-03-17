#ifndef _LOCKS_H_
#define _LOCKS_H_

#include "Lists.h"
#include "Threads.h"

namespace Locks {
	
class InterruptsLock {
private:
	bool fOldEnabled, fLocked;

public:
	InterruptsLock(bool doLock = true);
	~InterruptsLock();
	void Acquire();
	void Release();
};


class Spinlock
{
private:
	uint32_t fValue;
	bool fOldIntsEnabled;

public:
	Spinlock();
	void Acquire();
	void Release();
};


struct WaitList: public Lists::List
{
	Threads::Thread *thread;
	uint32_t waiting;
	
	void Acquire();
	void Release();
};

enum State {
	acquired = 1 << 0,
	waiting = 1 << 1,
};

class Mutex
{
private:
	uint32_t fValue;
	Lists::List fWaitList;

public:
	Mutex(bool acquireFirst = false);
	bool Acquire();
	void Release(bool releaseAll = false);
	void ReleaseIfWaiting(bool releaseAll = false);
	bool SwitchFrom(Mutex &src);
};

}

#endif	// _LOCKS_H_
