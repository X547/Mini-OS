#include "Views.h"
#include <stdlib.h>

namespace Views {

Port *gPort = NULL;


//#pragma mark Port

View *&Port::ThisFocus(Focus focus)
{
	switch (focus) {
		case mainFocus: return fMainFocus;
		case pointerFocus: return fPointerFocus;
	}
}

void Port::SetFocus(Focus focus, View *view)
{
	View *&focusView = ThisFocus(focus);
	if (focusView == view) return;
	// WriteString("SetFocus(0x"); WriteHex((size_t)view, 0); WriteString(")"); WriteLn();
 	if (focusView != NULL) {
		FocusMsg msg(focus, false); focusView->HandleMsg(msg);
	}
	focusView = view;
	if (focusView != NULL) {
		FocusMsg msg(focus, true); focusView->HandleMsg(msg);
		SetCursor(focusView->GetCursor());
	}
}

void Port::HandleMsg(Message &_msg)
{
	if (PointerMsg* msg = dynamic_cast<PointerMsg*>(&_msg)) {
		// WriteString("PointerMsg(("); WriteInt(msg->pos.x); WriteString(", "); WriteInt(msg->pos.y); WriteString("), "); WriteSet(msg->btns); WriteString(")"); WriteLn();
		if (msg->btns == 0) fPointerCaptured = false;
		Point pos;
		if (!fPointerCaptured) {
			SetFocus(pointerFocus, fRoot->ViewAt(msg->pos, &pos));
		} else {
			pos = msg->pos;
			for (View *it = fPointerFocus; it != NULL; it = it->fUp) {
				pos -= it->fRect.LeftTop();
			}
		}
		if (fPointerFocus == NULL) return;
		msg->pos = pos;
		fPointerFocus->HandleMsg(_msg);
	} else {
		if (fMainFocus == NULL) return;
		fMainFocus->HandleMsg(_msg);
	}
}

Port::Port(): fRoot(NULL), fMainFocus(NULL), fPointerFocus(NULL)
{}

void Port::SetRoot(View *src, int32_t w, int32_t h)
{
	if (fRoot != NULL) {
		fRoot->fPort = NULL;
		fRoot->fRect.MakeEmpty();
	}
	fRoot = src;
	if (fRoot != NULL) {
		fRoot->fPort = this;
		fRoot->fRect = Rect(0, 0, w, h);
		fDirty = fRoot->fRect;
	} else {
		fDirty.MakeEmpty();
	}
}

void Port::Invalidate(Rect dirty)
{
	// WriteString("Port::Invalidate("); dirty.Write(); WriteString(")"); WriteLn();
	fDirty |= dirty;
}

void Port::Validate()
{
	fDirty.MakeEmpty();
}

void Port::DrawInt(View *v, RasBufOfs32 rb)
{
	v->Draw(rb, v->Bounds());
	for (View *it = (View*)v->fDown.fNext; it != &v->fDown; it = (View*)it->fNext) {
		RasBufOfs32 rb2 = rb.ClipOfs(it->fRect);
		rb2.pos += it->fRect.LeftTop();
		DrawInt(it, rb2);
	}
}

void Port::Draw(RasBufOfs32 rb)
{
	DrawInt(fRoot, rb);
}


//#pragma mark View

View::View(): fCursor(&gPointer)
{
	fDown.fPrev = &fDown; fDown.fNext = &fDown;
}

void View::Attach(Rect rc, View *src, View *prev)
{
	Assert(src != NULL);
	Assert(src->fUp == NULL);
	Assert(prev == NULL || prev->fUp == this);
	src->fPort = fPort;
	src->fUp = this;
	src->fRect = rc; src->fRect.Normalize();
	Invalidate(src->fRect);
	if (prev == NULL) prev = (View*)fDown.fPrev;
	prev->InsertAfter(src);
	src->Resized(src->fRect.Width(), src->fRect.Height());
}

void View::Detach(View *src)
{
	Assert(src != NULL);
	Assert(src->fUp == this);
	Invalidate(src->fRect);
	src->Resized(0, 0);
	fDown.Remove(this);
	src->fUp = NULL;
	src->fPort = NULL;
}

Rect View::GetRect(View *src)
{
	Assert(src != NULL);
	Assert(src->fUp == this);
	return src->fRect;
}

void View::SetRect(View *src, Rect rc)
{
	Assert(src != NULL);
	Assert(src->fUp == this);
	if (rc == src->fRect) return;
	Invalidate(src->fRect);
	src->fRect = rc;
	Invalidate(src->fRect);
	src->Resized(src->fRect.Width(), src->fRect.Height());
}

Rect View::Bounds() const
{
	return fRect - fRect.LeftTop();
}

View *View::ViewAt(Point pt, Point *viewPt)
{
	if (!Bounds().Contains(pt)) return NULL;
	for (View *it = (View*)fDown.fPrev; it != &fDown; it = (View*)it->fPrev) {
		View *down = it->ViewAt(pt - it->fRect.LeftTop(), viewPt);
		if (down != NULL) return down;
	}
	if (viewPt != NULL) {*viewPt = pt;}
	return this;
}

void View::SetCursor(Cursor *cursor)
{
	if (fCursor == cursor) return;
	fCursor = cursor;
	if (fPort != NULL && fPort->ThisFocus(pointerFocus) == this) {
		SetCursor(fCursor);
	}
}

void View::SetCapture(bool doSet)
{
	if (fPort->ThisFocus(pointerFocus) == this) {
		fPort->fPointerCaptured = doSet;
	}
}

void View::Invalidate(Rect dirty)
{
	// WriteString("View::Invalidate("); dirty.Write(); WriteString(")"); WriteLn();
	if (fPort == NULL) {WriteString("fPort == NULL"); WriteLn(); return;}
	for (View *it = this; it != NULL; it = it->fUp) {
		dirty &= it->Bounds();
		dirty += it->fRect.LeftTop();
	}
	fPort->Invalidate(dirty);
}

void View::Resized(int32_t w, int32_t h)
{}

void View::Draw(RasBufOfs32 rb, Rect dirty)
{}

void View::HandleMsg(Message &_msg)
{}


void Draw()
{
	if (gPort != NULL) {
		Rect dirty = gPort->DirtyRect();
		// WriteString("dirty: "); dirty.Write(); WriteLn();
		gPort->Validate();
		RasBufOfs32 rb = ((RasBufOfs32)gBackBuf).ClipOfs(dirty);
		gPort->Draw(rb);
		FlushBackBuf(dirty);
	}
}

}
