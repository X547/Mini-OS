#ifndef _TEXTMODELS_H_
#define _TEXTMODELS_H_

#include "AutoDeleter.h"
#include "Lists.h"
#include "Graphics.h"

namespace TextModels {

class Attributes {
public:
	Font *fFont;
	uint32_t fColor;

	bool operator ==(const Attributes &src) const {return fFont == src.fFont && fColor == src.fColor;}
	bool operator !=(const Attributes &src) const {return !(*this == src);}
};

class Run: public Lists::List {
public:
	uint64_t fLen;
	size_t fDataLen;
	ArrayDeleter<char> fData;
	Attributes fAttrs;
	
	void Insert(size_t ofs, const char *chars, size_t len);
	void Remove(size_t ofs, size_t len);
};

class Rider {
private:
	class Model *fBase;
	uint64_t fEra;
	uint64_t fPos;
	Run *fRun;
	uint64_t fOfs;

public:
	Rider();
	Rider(const Rider &src);

	uint64_t Pos();
	void SetPos(uint64_t pos);
	Rider &operator +=(int64_t ofs);
	Rider &operator -=(int64_t ofs);
	Rider operator +(int64_t ofs) const;
	Rider operator -(int64_t ofs) const;
};

class Model {
private:
	Run fTrailer;
	uint64_t fLen;
	uint32_t fEra;

public:
	Model();
	~Model();

	uint64_t Length() const {return fLen;}
	Rider Begin() const;
	Rider End() const;

	void Delete(const Rider &rd, uint64_t len);
	Rider Read(const Rider &rd, char *chars, size_t len);
	Rider ReadRun(const Rider &rd, Attributes &attrs);
	Rider Write(const Rider &rd, const char *chars, size_t len, const Attributes &attrs);
};

extern Attributes gDefAttrs;

}

#endif	// _TEXTMODELS_H_
