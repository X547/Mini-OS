#include "Graphics.h"
#include "Htif.h"
#include "Modules.h"
#include <string.h>
#include <new>


RasBuf32 gFrontBuf = {NULL, 0, 0, 0}, gBackBuf;
int32_t gCarX0;
Point gCarPos;

Cursor *gCursor;
Point gCursorPos;
bool gCursorVisible;
uint32_t gCursorBtns = 0;
uint32_t gUnderCursorData[32*32];
RasBuf32 gUnderCursorRb = {gUnderCursorData, 32, 32, 32};


//#pragma mark RasBuf

void Clear(RasBuf32 vb, uint32_t c)
{
	vb.stride -= vb.width;
	for (; vb.height > 0; vb.height--) {
		for (int x = 0; x < vb.width; x++) {
			*vb.colors = c;
			vb.colors++;
		}
		vb.colors += vb.stride;
	}
}

template <typename Color>
RasBuf<Color> RasBuf<Color>::Clip(int x, int y, int w, int h) const
{
	RasBuf<Color> vb = *this;
	if (x < 0) {w += x; x = 0;}
	if (y < 0) {h += y; y = 0;}
	if (x + w > vb.width) {w = vb.width - x;}
	if (y + h > vb.height) {h = vb.height - y;}
	if (w > 0 && h > 0) {
		vb.colors += y*vb.stride + x;
		vb.width = w;
		vb.height = h;
	} else {
		vb.colors = 0;
		vb.width = 0;
		vb.height = 0;
	}
	return vb;
}

template <typename Color>
RasBuf<Color> RasBuf<Color>::Clip(Rect rc) const
{
	return Clip(rc.l, rc.t, rc.Width(), rc.Height());
}

//template void Clip<RasBuf8>(RasBuf8 &vb, int x, int y, int w, int h);
//template void Clip<RasBuf32>(RasBuf32 &vb, int x, int y, int w, int h);

template <typename Color>
RasBufOfs<Color> RasBufOfs<Color>::ClipOfs(Rect rc) const
{
	RasBufOfs<Color> vb = this->Clip(rc.l + pos.x, rc.t + pos.y, rc.Width(), rc.Height());
	vb.pos.x = std::min(pos.x, -rc.l);
	vb.pos.y = std::min(pos.y, -rc.t);
	return vb;
}

//template void Clip<RasBufOfs8>(RasBufOfs8 &vb, int l, int t, int r, int b);
//template void Clip<RasBufOfs32>(RasBufOfs32 &vb, int l, int t, int r, int b);

void FillRect(RasBuf32 vb, Rect rc, uint32_t c)
{
	Clear(vb.Clip(rc), c);
}

void StrokeRect(RasBuf32 vb, Rect rc, int s, uint32_t c)
{
	if (rc.Width() <= 2*s || rc.Height() <= 2*s)
		FillRect(vb, rc, c);
	else {
		FillRect(vb, Rect(rc.l,     rc.t,     rc.r,     rc.t + s), c);
		FillRect(vb, Rect(rc.l,     rc.t + s, rc.l + s, rc.b - s), c);
		FillRect(vb, Rect(rc.r - s, rc.t + s, rc.r,     rc.b - s), c);
		FillRect(vb, Rect(rc.l,     rc.b - s, rc.r,     rc.b    ), c);
	}
}

void FillRect(RasBufOfs32 vb, Rect rc, uint32_t c)
{
	FillRect((RasBuf32)vb, rc + vb.pos, c);
}

void StrokeRect(RasBufOfs32 vb, Rect rc, int s, uint32_t c)
{
	StrokeRect((RasBuf32)vb, rc + vb.pos, s, c);
}


void Blit(RasBuf32 dst, RasBuf32 src, Point pos)
{
	int dstW, dstH;
	dstW = dst.width; dstH = dst.height;
	dst = dst.Clip(pos.x, pos.y, src.width, src.height);
	src = src.Clip(-pos.x, -pos.y, dstW, dstH);
	for (; dst.height > 0; dst.height--) {
		memcpy(dst.colors, src.colors, dst.width*sizeof(*dst.colors));
		dst.colors += dst.stride;
		src.colors += src.stride;
	}
}

void Blit(RasBufOfs32 dst, RasBufOfs32 src)
{
	Blit(dst, src, dst.pos - src.pos);
}

void BlitRgb(RasBuf32 dst, RasBuf32 src, Point pos)
{
	int dstW, dstH;
	dstW = dst.width; dstH = dst.height;
	dst = dst.Clip(pos.x, pos.y, src.width, src.height);
	src = src.Clip(-pos.x, -pos.y, dstW, dstH);
	dst.stride -= dst.width;
	src.stride -= src.width;
	for (; dst.height > 0; dst.height--) {
		for (pos.x = dst.width; pos.x > 0; pos.x--) {
			uint32_t dc = *dst.colors;
			uint32_t sc = *src.colors;
			uint32_t a = sc >> 24;
			if (a != 0) {
				*dst.colors =
					((((dc >>  0) % 0x100)*(0xff - a)/0xff + ((sc >>  0) % 0x100)*a/0xff) <<  0) +
					((((dc >>  8) % 0x100)*(0xff - a)/0xff + ((sc >>  8) % 0x100)*a/0xff) <<  8) +
					((((dc >> 16) % 0x100)*(0xff - a)/0xff + ((sc >> 16) % 0x100)*a/0xff) << 16) +
					(dc & 0xff000000);
			}
			dst.colors++;
			src.colors++;
		}
		dst.colors += dst.stride;
		src.colors += src.stride;
	}
}

void BlitRgb(RasBufOfs32 dst, RasBufOfs32 src)
{
	BlitRgb(dst, src, dst.pos - src.pos);
}

void BlitMask(RasBuf32 dst, RasBuf8 src, Point pos, uint32_t c)
{
	int dstW, dstH;
	uint32_t dc, sc, a;
	dstW = dst.width; dstH = dst.height;
	dst = dst.Clip(pos.x, pos.y, src.width, src.height);
	src = src.Clip(-pos.x, -pos.y, dstW, dstH);
	dst.stride -= dst.width;
	src.stride -= src.width;
	for (; dst.height > 0; dst.height--) {
		for (pos.x = dst.width; pos.x > 0; pos.x--) {
			dc = *dst.colors;
			sc = c % 0x1000000 + (*src.colors % 0x100 << 24);
			a = (dc >> 24) + (sc >> 24) - (dc >> 24)*(sc >> 24)/0xff;
			if (a != 0) {
				*dst.colors =
					(((0xff - (sc >> 24))*(dc >> 24)*((dc >>  0) % 0x100)/(0xff*a) + (sc >> 24)*((sc >>  0) % 0x100)/a) <<  0)  +
					(((0xff - (sc >> 24))*(dc >> 24)*((dc >>  8) % 0x100)/(0xff*a) + (sc >> 24)*((sc >>  8) % 0x100)/a) <<  8) +
					(((0xff - (sc >> 24))*(dc >> 24)*((dc >> 16) % 0x100)/(0xff*a) + (sc >> 24)*((sc >> 16) % 0x100)/a) << 16) +
					(a << 24);
			}
/*
			sc = 0xff - *src.colors;
			*dst.colors = sc + (sc << 8) + (sc << 16) + (0xff << 24);
*/
			dst.colors++;
			src.colors++;
		}
		dst.colors += dst.stride;
		src.colors += src.stride;
	}
}

void BlitMaskRgb(RasBuf32 dst, RasBuf8 src, Point pos, uint32_t c)
{
	int dstW, dstH;
	uint32_t dc, a, aInv;
	dstW = dst.width; dstH = dst.height;
	dst = dst.Clip(pos.x, pos.y, src.width, src.height);
	src = src.Clip(-pos.x, -pos.y, dstW, dstH);
	dst.stride -= dst.width;
	src.stride -= src.width;
	for (; dst.height > 0; dst.height--) {
		for (pos.x = dst.width; pos.x > 0; pos.x--) {
			dc = *dst.colors;
			a = *src.colors;
			if (a != 0) {
				*dst.colors =
					((((dc >>  0) % 0x100)*(0xff - a)/0xff + ((c >>  0) % 0x100)*a/0xff) <<  0) +
					((((dc >>  8) % 0x100)*(0xff - a)/0xff + ((c >>  8) % 0x100)*a/0xff) <<  8) +
					((((dc >> 16) % 0x100)*(0xff - a)/0xff + ((c >> 16) % 0x100)*a/0xff) << 16) +
					(dc & 0xff000000);
			}
			dst.colors++;
			src.colors++;
		}
		dst.colors += dst.stride;
		src.colors += src.stride;
	}
}


//#pragma mark Fonts

Glyph *ThisGlyph(const Font &f, uint32_t ch)
{
	uint32_t l, r, m;
	Glyph *glyphs = (Glyph*)((uint8_t*)&f + sizeof(Font));
	l = 0; r = f.glyphCnt;
	while (l < r) {
		m = (l + r)/2;
		if (ch < glyphs[m].ch)
			r = m;
		else if (ch > glyphs[m].ch)
			l = m + 1;
		else
			return &glyphs[m];
	}
	return ThisGlyph(f, '.');
}

void DrawString(RasBuf32 vb, Point &pos, uint32_t c, const char *str, const Font &font)
{
	uint8_t *data = (uint8_t*)&font + sizeof(Font) + font.glyphCnt*sizeof(Glyph) + font.kerningCnt*sizeof(Kerning);
	
	for (int i = 0; str[i] != '\0'; i++) {
		uint32_t ch = str[i]; // TODO: decode UTF-8
		if (ch == '\t') {
			pos.x += font.asc + font.dsc;
		} else {
			Glyph *gph = ThisGlyph(font, ch);
			RasBuf8 vb2 = {data + gph->ofs, gph->w, gph->w, gph->h};
			BlitMaskRgb(vb, vb2, pos + Point(gph->x, gph->y), c);
			pos.x += gph->adv;
		}
	}
}

void DrawString(RasBufOfs32 vb, Point &pos, uint32_t c, const char *str, const Font &font)
{
	pos += vb.pos;
	DrawString((RasBuf32)vb, pos, c, str, font);
	pos -= vb.pos;
}

int StringWidth(const char *str, const Font &font)
{
	int x = 0;
	for (int i = 0; str[i] != '\0'; i++) {
		uint32_t ch = str[i]; // TODO: decode UTF-8
		Glyph *gph = ThisGlyph(font, ch);
		x += gph->adv;
	}
	return x;
}


//#pragma mark Text output

void ResetCaret()
{
	gCarX0 = 0; gCarPos = Point(gCarX0, gFont.asc);
}

void WriteString(const char *str)
{
	OutString(str);
	//DrawString(gFrontBuf, gCarPos, 0xff000000, str, gFont);
}

void WriteString(const char *str, size_t len)
{
	OutString(str, len);
	//DrawString(gFrontBuf, gCarPos, 0xff000000, str, gFont);
}

void WriteChar(char ch)
{
	char str[2];
	str[0] = ch;
	str[1] = '\0';
	WriteString(str);
}

void WriteTab()
{
	WriteChar('\t');
}

void WriteLn()
{
	OutChar('\n');
/*
	gCarPos.x = gCarX0;
	gCarPos.y += gFont.asc + gFont.dsc;
*/
}

void WriteHex(uint64_t val, int n)
{
	if (n > 1 || val > 0xf) WriteHex(val/0x10, n - 1);
	val = val%0x10;
	if (val <= 9) WriteChar(val + '0');
	else WriteChar(val - 10 + 'A');
}

void WriteInt(int64_t val)
{
	if (val == -9223372036854775807LL - 1) WriteString("-9223372036854775808");
	else if (val < 0) {WriteString("-"); WriteInt(-val);}
	else {
		if (val > 9) WriteInt(val/10);
		WriteChar(val%10 + '0');
	}
}

void WriteSet(uint64_t val)
{
	bool first = true;
	WriteString("{");
	for (int i = 0; i < 64; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else WriteString(", ");
			WriteInt(i);
		}
	}
	WriteString("}");
}


//#pragma mark Cursors

void FadeCursor()
{
	if (gCursor == NULL || !gCursorVisible) return;
	Blit(gFrontBuf, gUnderCursorRb, gCursorPos + Point(gCursor->x, gCursor->y));
	gCursorVisible = false;
}

void ShowCursor()
{
	if (gCursor == NULL) return;
	RasBufOfs32 cursorRb = gCursor->ThisBuf();
	Blit(gUnderCursorRb, gFrontBuf, -(gCursorPos + cursorRb.pos));
	BlitRgb(gFrontBuf, cursorRb, gCursorPos + cursorRb.pos);
	gCursorVisible = true;
}

void SetCursor(Cursor *cursor)
{
	// WriteString("SetCursor("); Modules::WritePC((size_t)cursor); WriteString(")"); WriteLn();
	if (gCursor == cursor) return;
	FadeCursor();
	gCursor = cursor;
	ShowCursor();
}

void MoveCursor(int32_t x, int32_t y)
{
	FadeCursor();
	//Point oldPos = gCursorPos;
	gCursorPos.x = x; gCursorPos.y = y;
	//FlushBackBuf(gCursor->ThisBuf().Frame() + oldPos);
	//FlushBackBuf(gCursor->ThisBuf().Frame() + gCursorPos);
	ShowCursor();
}


//#pragma mark -

void FlushBackBuf(Rect rect)
{
	RasBufOfs32 clip = ((RasBufOfs32)gBackBuf).ClipOfs(rect);
	// WriteString("clip.Frame(): "); clip.Frame().Write(); WriteLn();
	if (gCursor != NULL && gCursorVisible && rect.Intersects(gCursor->ThisBuf().Frame() + gCursorPos)) {
		RasBufOfs32 cursorRb = gCursor->ThisBuf();
		RasBufOfs32 underCursorRb = ((RasBufOfs32)gUnderCursorRb) - cursorRb.pos - gCursorPos;
		//Clear(underCursorRb, 0xff000000);
		Blit(underCursorRb, clip);
		BlitRgb(gBackBuf, cursorRb, gCursorPos + cursorRb.pos);
	}
	Blit(gFrontBuf, clip);
}

namespace Graphics {

void Init()
{
	Clear(gFrontBuf, 0xffffffff);

	gBackBuf.stride = gFrontBuf.width;
	gBackBuf.width = gFrontBuf.width;
	gBackBuf.height = gFrontBuf.height;
	gBackBuf.colors = new(std::nothrow) uint32_t[gBackBuf.stride*gBackBuf.height];
}

}
