#include "Buttons.h"
#include <string.h>
#include <new>


void Button::SetFlags(uint32_t flags)
{
	if (fFlags == flags) return;
	Invalidate(Bounds());
	fFlags = flags;
}


Button::Button(const char *label, Action *action): fMainFocus(false), fPointerFocus(false), fTrack(false), fBtns(0), fFlags(0), fAction(action)
{
	fLabel.SetTo(new(std::nothrow) char[strlen(label + 1)]);
	strcpy(fLabel.Get(), label);
}

Button::~Button()
{
}


void Button::Enable(bool doEnable)
{
	if(doEnable)
		SetFlags(fFlags & ~(1 << offFlag));
	else {
		SetFlags(fFlags | (1 << offFlag));
		fTrack = false;
		SetCapture(false);
	}
}


void Button::Draw(RasBufOfs32 rb, Rect dirty)
{
	uint32_t c1, c2, cLabel;
	
	if (((1 << offFlag) & fFlags) || (!((1 << insideFlag) & fFlags) && !((1 << downFlag) & fFlags))) {
		c1 = 0xffffffff;
		c2 = 0xffcccccc;
	} else if (((1 << insideFlag) & fFlags) && ((1 << downFlag) & fFlags)) {
		c1 = 0xffeeeeee;
		c2 = 0xff666666;
	} else {
		c1 = 0xffffffff;
		c2 = 0xff999999;
	}
	if (!((1 << offFlag) & fFlags))
		cLabel = 0xff000000;
	else
		cLabel = 0xff999999;

	FillRect(rb, Bounds(), c1);
	StrokeRect(rb, Bounds(), 1, c2);
	if ((1 << focusFlag) & fFlags)
		StrokeRect(rb, Bounds().Inset(2, 2), 1, 0x000099ff);


	int32_t labelW, labelH;
	labelW = StringWidth(fLabel.Get(), gFont);
	labelH = gFont.asc + gFont.dsc;
	Point strPos = Point(Bounds().Width()/2, Bounds().Height()/2) - Point(labelW/2, labelH/2) + Point(0, gFont.asc);
	DrawString(rb, strPos, cLabel, fLabel.Get(), gFont);
}

void Button::HandleMsg(Views::Message &_msg)
{
	if (auto msg = dynamic_cast<Views::FocusMsg*>(&_msg)) {
		switch (msg->focus) {
		case Views::mainFocus:
			fMainFocus = msg->on;
			if (msg->on)
				SetFlags(fFlags | (1 << focusFlag));
			else
				SetFlags(fFlags & ~(1 << focusFlag));
			break;
		case Views::pointerFocus:
			if (!msg->on) {
				fPointerFocus = false;
				fTrack = false;
				SetFlags(fFlags & ~((1 << insideFlag) | (1 << downFlag)));
			}
			break;
		}
	} else if (auto msg = dynamic_cast<Views::PointerMsg*>(&_msg)) {
		if (!fPointerFocus) {
			fPointerFocus = true;
			SetFlags(fFlags | (1 << insideFlag));
		} else if (!(fFlags & (1 << offFlag))) {
			if (!fTrack) {
				if (fBtns == 0 && msg->btns != 0) {
					SetFlags(fFlags | (1 << downFlag));
					fTrack = true;
					SetCapture(true);
					GetPort()->SetFocus(Views::mainFocus, this);
				}
			} else {
				if (Bounds().Contains(msg->pos))
					SetFlags(fFlags | (1 << insideFlag));
				else
					SetFlags(fFlags & ~(1 << insideFlag));

				if (msg->btns == 0) {
					SetFlags(fFlags & ~(1 << downFlag));
					fTrack = false;
					SetCapture(false);
					if (fFlags & (1 << insideFlag) && fAction.IsSet())
						fAction->Do();
				}
			}
		}
		fBtns = msg->btns;
	} else if (auto msg = dynamic_cast<Views::KeyMsg*>(&_msg)) {
		if (!(fFlags & (1 << offFlag)))
			if (msg->key == 71 /* enter */ && msg->isDown && fAction.IsSet())
				fAction->Do();
	}
}
