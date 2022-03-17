#ifndef _TEXTMAPPERS_H_
#define _TEXTMAPPERS_H_

namespace TextMappers {

class Formatter {
public:
	virtual void String(const char *str) = 0;
	virtual void Char(char ch) = 0;
	virtual void Tab() = 0;
	virtual void Ln() = 0;
	virtual void Hex(uint64_t val, int n) = 0;
	virtual void Int(int64_t val) = 0;
	virtual void Set(uint64_t val) = 0;
};

};

#endif	// _TEXTMAPPERS_H_
