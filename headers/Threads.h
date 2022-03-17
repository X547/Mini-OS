#ifndef _THREADS_H_
#define _THREADS_H_

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include "Lists.h"


namespace Threads {

class Thread;

static inline Thread *Current() {Thread *th; asm volatile("mv %0, tp" : "=r" (th)); return th;}

void Yield();
void Wait();
[[noreturn]] void Exit(int32_t res);
void Wake(Thread *th, bool doSwitch);
int32_t Join(Thread *th);

void Dump();


enum State {inactive, done, waiting, ready, running};

class Thread: private Lists::List
{
private:
	char fName[32];
	State fState;
	int32_t fRes;
	Thread *fJoin;
	void *fStack;
	size_t fStackSize;
	jmp_buf fContext;
	
	friend void Switch(Thread *th, State state);
	friend void Exit(int32_t res);
	friend void Wake(Thread *th, bool doSwitch);
	friend int32_t Join(Thread *th);
	friend void Dump();

public:
	Thread(const char *name, size_t stackSize, int32_t (*Entry)(void *arg), void *arg);
	Thread(const char *name, void *stack, size_t stackSize);
	~Thread();
	
	const char *Name() {return fName;}
};

}

#endif	// _THREADS_H_
