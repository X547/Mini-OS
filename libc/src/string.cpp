#include <string.h>
#include <stdint.h>

// from xv6-riscv ulib.c

char* strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

char* strlcpy(char *dst, const char *src, size_t size)
{
	size_t i;
	for (i = 0; src[i] != '\0' && i < size - 1; i++)
		dst[i] = src[i];
	dst[i] = '\0';
	return dst;
}

int strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (unsigned char)*p - (unsigned char)*q;
}

unsigned strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}
#ifndef __riscv
void* memset(void *dst, int c, size_t n)
{
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}
#endif
char* strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

int atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void* memmove(void *vdst, const void *vsrc, ssize_t n)
{
  char *dst;
  const char *src;

  dst = (char*)vdst;
  src = (const char*)vsrc;
  if (src > dst) {
    while(n-- > 0)
      *dst++ = *src++;
  } else {
    dst += n;
    src += n;
    while(n-- > 0)
      *--dst = *--src;
  }
  return vdst;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  const char *p1 = (const char*)s1, *p2 = (const char*)s2;
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}
#ifndef __riscv
void *memcpy(void *dst, const void *src, size_t n)
{
  return memmove(dst, src, n);
}
#endif

void *memchr(void const *buf, int c, size_t len)
{
	unsigned char const *b = (unsigned char const*)buf;
	unsigned char x = (c&0xff);
	size_t i;

	for (i = 0; i < len; i++) {
		if (b[i] == x)
			return (void*)(b + i);
	}

	return NULL;
}
