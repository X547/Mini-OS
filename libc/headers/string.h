#ifndef _STRING_H_
#define _STRING_H_

#include <stddef.h>

extern "C" {

char* strcpy(char *s, const char *t);
char* strlcpy(char *dst, const char *src, size_t size);
int strcmp(const char *p, const char *q);
unsigned strlen(const char *s);
void* memset(void *dst, int c, size_t n);
char* strchr(const char *s, char c);
int atoi(const char *s);
void* memmove(void *vdst, const void *vsrc, ssize_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memchr(void const *buf, int c, size_t len);

}

#endif	// _STRING_H_
