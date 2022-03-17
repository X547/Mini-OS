#ifndef _STDDEF_H_
#define _STDDEF_H_

typedef unsigned long size_t;
typedef long ssize_t;
typedef long ptrdiff_t;

#define NULL 0L
#define offsetof(type, member) __builtin_offsetof(type, member)

#define BasePtr(ptr, Type, member) (Type*)((char*)ptr - offsetof(Type, member))

#define ALWAYS_INLINE __attribute__((always_inline))

#define RoundDown(a, b)	(((a) / (b)) * (b))
#define RoundUp(a, b)	RoundDown((a) + (b) - 1, b)

#endif	// _STDDEF_H_
