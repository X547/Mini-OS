#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <stdint.h>


typedef struct {
	uint64_t ra;
	uint64_t s[12];
	uint64_t sp;
	uint64_t fs[12];
} jmp_buf[1];

extern "C" {

extern int setjmp(jmp_buf);
extern void longjmp(jmp_buf, int) __attribute__ ((__noreturn__));

}

#endif	// _SETJMP_H_
