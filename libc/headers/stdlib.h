#ifndef _STDLIB_H_
#define _STDLIB_H_

extern "C" {

[[noreturn]] void abort();

}

inline void Assert(bool cond) {if (!cond) abort();}

#endif	// _STDLIB_H_
