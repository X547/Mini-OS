#include "Unicode.h"

namespace Unicode {

int CharLen8(char ch)
{
	if (ch < 0xc0) return 1;
	if (ch < 0xe0) return 2;
	if (ch < 0xf0) return 3;
	if (ch < 0xf8) return 4;
	if (ch < 0xfc) return 5;
	if (ch <= 0xfd) return 6;
	return 1;
}

int CharLenFromCode8(uint32_t code)
{
	if (code < 0x00000080) return 1;
	if (code < 0x00000800) return 2;
	if (code < 0x00010000) return 3;
	if (code < 0x00200000) return 4;
	if (code < 0x04000000) return 5;
	if (code <= 0x7fffffff) return 6;
	return 1;
}

}
