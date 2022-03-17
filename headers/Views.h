#ifndef _VIEWS_H_
#define _VIEWS_H_

#include "Lists.h"
#include "Graphics.h"
#include <stdint.h>

namespace Views {

struct Message {
	virtual ~Message() {}
};

enum Focus {mainFocus, pointerFocus};

struct FocusMsg: public Message {
	Focus focus;
	bool on;
	FocusMsg(Focus focus, bool on): focus(focus), on(on) {}
};

struct PointerMsg: public Message {
	Point pos;
	uint32_t btns;
	PointerMsg(Point pos, uint32_t btns): pos(pos), btns(btns) {}
};

struct KeyMsg: public Message {
	int32_t key;
	bool isDown;
	KeyMsg(int32_t key, bool isDown): key(key), isDown(isDown) {}
};

class View;

class Port
{
private:
	Rect fDirty;
	View *fRoot;
	View *fMainFocus;
	View *fPointerFocus;
	bool fPointerCaptured;
	friend class View;
	
	void DrawInt(View *v, RasBufOfs32 rb);

public:
	Port();
	View *Root() const {return fRoot;}
	void SetRoot(View *src, int32_t w, int32_t h);
	View *&ThisFocus(Focus focus);
	void SetFocus(Focus focus, View *view);
	const Rect &DirtyRect() const {return fDirty;}
	void Invalidate(Rect dirty);
	void Validate();
	void Draw(RasBufOfs32 rb);
	void HandleMsg(Message &msg);
};


class View: public Lists::List
{
private:
	Port *fPort;
	View *fUp;
	List fDown;
	Rect fRect;
	Cursor *fCursor;
	friend class Port;
	
public:
	View();
	
	Port *GetPort() const {return fPort;}
	View *Up() const {return fUp;}
	void Attach(Rect rc, View *src, View *prev = NULL);
	void Detach(View *src);
	Rect GetRect(View *src);
	void SetRect(View *src, Rect rc);
	Rect Bounds() const;
	View *ViewAt(Point pt, Point *viewPt = NULL);
	Cursor *GetCursor() const {return fCursor;}
	void SetCursor(Cursor *cursor);
	void SetCapture(bool doSet);
	void Invalidate(Rect dirty);
	virtual void Resized(int32_t w, int32_t h);
	virtual void Draw(RasBufOfs32 rb, Rect dirty);
	virtual void HandleMsg(Message &msg);
};

extern Port *gPort;

void Draw();

}

#endif	// _VIEWS_H_
