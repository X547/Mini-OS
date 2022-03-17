#ifndef _UNICODE_H_
#define _UNICODE_H_

namespace Unicode {

int CharLen8(char ch);
int CharLenFromCode8(uint32_t code);
void Encode8(uint32_t code, char *str, size_t &pos);
uint32_t Decode8(const char *str, size_t &pos);

}

#endif	// _UNICODE_H_
