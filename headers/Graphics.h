#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <stddef.h>
#include <stdint.h>

// #include <algorithm>
#include <bits/move.h>
namespace std {
template <typename Type>
Type min(Type a, Type b) {if (a < b) return a; return b;}

template <typename Type>
Type max(Type a, Type b) {if (a > b) return a; return b;}
/*
template <typename Type>
void swap(Type &a, Type &b) {Type c; c = a; a = b; b = c;}
*/
}


void WriteString(const char *str);
void WriteString(const char *str, size_t len);
void WriteChar(char ch);
void WriteTab();
void WriteLn();
void WriteHex(uint64_t val, int n);
void WriteInt(int64_t val);
void WriteSet(uint64_t val);


struct Point {
	int32_t x, y;

	Point(): x(0), y(0) {}
	Point(int32_t x, int32_t y): x(x), y(y) {}

	Point &operator +=(const Point &pt2)
	{
		x += pt2.x; y += pt2.y;
		return *this;
	}

	Point &operator -=(const Point &pt2)
	{
		x -= pt2.x; y -= pt2.y;
		return *this;
	}

	Point operator +(const Point &pt2) const
	{
		Point res = *this;
		res += pt2;
		return res;
	}

	Point operator -(const Point &pt2) const
	{
		Point res = *this;
		res -= pt2;
		return res;
	}

	Point operator -() const
	{
		return Point(-x, -y);
	}

	void Write() const
	{
		WriteString("(");
		WriteInt(x); WriteString(", ");
		WriteInt(y); WriteString(")");
	}
};

struct Rect {
	int32_t l, t, r, b;

	Rect(): l(0), t(0), r(0), b(0) {}
	Rect(int32_t l, int32_t t, int32_t r, int32_t b): l(l), t(t), r(r), b(b) {}
	Rect(int32_t w, int32_t h): l(0), t(0), r(w), b(h) {}

	bool IsEmpty() const
	{
		return !(l < r && t < b);
	}

	void MakeEmpty()
	{
		l = 0; t = 0; r = 0; b = 0;
	}

	bool operator ==(const Rect &r2) const
	{
		return l == r2.l && t == r2.t && r == r2.r && b == r2.b;
	}

	bool Contains(const Point &pt) const
	{
		return pt.x >= l && pt.y >= t && pt.x < r && pt.y < b;
	}

	bool Contains(const Rect &rc2) const
	{
		return rc2.l >= l && rc2.t >= t && rc2.r <= r && rc2.b <= b;
	}

	bool Intersects(const Rect &rc2) const
	{
		return std::max(l, rc2.l) < std::min(r, rc2.r) && std::max(t, rc2.t) < std::min(b, rc2.b);
	}

	int32_t Width() const
	{
		return r - l;
	}

	int32_t Height() const
	{
		return b - t;
	}

	Point LeftTop() const
	{
		Point pt = {l, t};
		return pt;
	}

	Point RightBottom() const
	{
		Point pt = {r, b};
		return pt;
	}

	void Normalize()
	{
		if (l > r) std::swap(l, r);
		if (t > b) std::swap(t, b);
	}

	Rect &operator &=(const Rect &r2)
	{
		l = std::max(l, r2.l);
		t = std::max(t, r2.t);
		r = std::min(r, r2.r);
		b = std::min(b, r2.b);
		if (!(l <= r && t <= b)) MakeEmpty();
		return *this;
	}

	Rect &operator |=(const Rect &r2)
	{
		if (IsEmpty()) {
			*this = r2;
		} else {
			l = std::min(l, r2.l);
			t = std::min(t, r2.t);
			r = std::max(r, r2.r);
			b = std::max(b, r2.b);
		}
		return *this;
	}

	Rect &operator +=(const Point &ofs)
	{
		l += ofs.x; t += ofs.y;
		r += ofs.x; b += ofs.y;
		return *this;
	}

	Rect &operator -=(const Point &ofs)
	{
		l -= ofs.x; t -= ofs.y;
		r -= ofs.x; b -= ofs.y;
		return *this;
	}

	Rect operator &(const Rect &r2) const
	{
		Rect res = *this;
		res &= r2;
		return res;
	}

	Rect operator |(const Rect &r2) const
	{
		if (IsEmpty()) return r2;
		return Rect(
			std::min(l, r2.l),
			std::min(t, r2.t),
			std::max(r, r2.r),
			std::max(b, r2.b)
		);
	}

	Rect operator +(const Point &ofs) const
	{
		Rect res = *this;
		res += ofs;
		return res;
	}

	Rect operator -(const Point &ofs) const
	{
		Rect res = *this;
		res -= ofs;
		return res;
	}

	Rect operator -() const
	{
		return Rect(-l, -t, -r, -b);
	}

	Rect Inset(int32_t dx, int32_t dy) const
	{
		return Rect(l + dx, t + dy, r - dx, b - dy);
	}
	
	void Write() const
	{
		WriteString("(");
		WriteInt(l); WriteString(", ");
		WriteInt(t); WriteString(", ");
		WriteInt(r); WriteString(", ");
		WriteInt(b); WriteString(")");
	}
};


struct __attribute__((packed)) Font {
	int32_t asc, dsc;
	int32_t glyphCnt;
	int32_t kerningCnt;
	int32_t dataLen;
};

struct __attribute__((packed)) Glyph {
	int32_t ch;
	int16_t adv;
	int16_t x, y;
	int16_t w, h;
	int32_t ofs;
};

struct __attribute__((packed)) Kerning {
	int32_t ch1, ch2;
	int32_t d;
};


template <typename Color>
struct RasBuf {
	Color *colors;
	int stride, width, height;
	
	RasBuf<Color> Clip(int x, int y, int w, int h) const;
	RasBuf<Color> Clip(Rect rc) const;
};

typedef RasBuf<uint8_t>  RasBuf8;
typedef RasBuf<uint32_t> RasBuf32;

template <typename Color>
struct RasBufOfs: public RasBuf<Color> {
	Point pos; // relative to dst
	
	RasBufOfs() {}
	RasBufOfs(const RasBuf<Color> &src) {*this = src;}
	RasBufOfs(const RasBufOfs<Color> &src) {*this = src;}
	
	RasBufOfs &operator =(const RasBuf<Color> &src) {this->colors = src.colors; this->stride = src.stride; this->width = src.width; this->height = src.height; pos.x = 0; pos.y = 0; return *this;}
	RasBufOfs &operator =(const RasBufOfs<Color> &src) {this->colors = src.colors; this->stride = src.stride; this->width = src.width; this->height = src.height; pos = src.pos; return *this;}

	RasBufOfs operator +(Point ofs) const {RasBufOfs res = *this; res.pos += ofs; return res;}
	RasBufOfs operator -(Point ofs) const {RasBufOfs res = *this; res.pos -= ofs; return res;}

	Rect Frame()
	{
		return Rect(this->width, this->height) + pos;
	}

	RasBufOfs<Color> ClipOfs(Rect rc) const;
};

typedef RasBufOfs<uint8_t>  RasBufOfs8;
typedef RasBufOfs<uint32_t> RasBufOfs32;


struct __attribute__((packed)) Cursor {
	int32_t w, h, x, y;
	uint32_t colors[0];
	
	RasBufOfs32 ThisBuf()
	{
		RasBufOfs32 rb;
		rb.colors = colors;
		rb.stride = w;
		rb.width  = w;
		rb.height = h;
		rb.pos.x  = x;
		rb.pos.y  = y;
		return rb;
	}
	
};


extern Font gFont;
extern Cursor gPointer, gIBeam;
extern RasBuf32 gFrontBuf, gBackBuf;
extern int32_t gCarX0;
extern Point gCarPos;

extern Cursor *gCursor;
extern Point gCursorPos;
extern bool gCursorVisible;
extern uint32_t gCursorBtns;


void Clear(RasBuf32 vb, uint32_t c);
/*
template <typename RasBuf>
void Clip(RasBuf &vb, int x, int y, int w, int h);

template <typename RasBufOfs>
void Clip2(RasBufOfs &vb, int l, int t, int r, int b);
*/
void FillRect(RasBuf32 vb, Rect rc, uint32_t c);
void StrokeRect(RasBuf32 vb, Rect rc, int s, uint32_t c);
void FillRect(RasBufOfs32 vb, Rect rc, uint32_t c);
void StrokeRect(RasBufOfs32 vb, Rect rc, int s, uint32_t c);

void Blit(RasBuf32 dst, RasBuf32 src, Point pos);
void Blit(RasBufOfs32 dst, RasBufOfs32 src);
void BlitRgb(RasBuf32 dst, RasBuf32 src, Point pos);
void BlitRgb(RasBufOfs32 dst, RasBufOfs32 src);
void BlitMask(RasBuf32 dst, RasBuf8 src, Point pos, uint32_t c);
void BlitMaskRgb(RasBuf32 dst, RasBuf8 src, Point pos, uint32_t c);
Glyph *ThisGlyph(const Font &f, uint32_t ch);
void DrawString(RasBuf32 vb, Point &pos, uint32_t c, const char *str, const Font &font);
void DrawString(RasBufOfs32 vb, Point &pos, uint32_t c, const char *str, const Font &font);
int StringWidth(const char *str, const Font &font);

void ResetCaret();

void FadeCursor();
void ShowCursor();
void SetCursor(Cursor *cursor);
void MoveCursor(int32_t x, int32_t y);

void FlushBackBuf(Rect rect);

namespace Graphics {
void Init();
}

#endif	// _GRAPHICS_H_
