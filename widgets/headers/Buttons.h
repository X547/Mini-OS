#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include "Views.h"
#include "AutoDeleter.h"


class Action
{
public:
	virtual ~Action() {}
	virtual void Do() = 0;
};

class Button: public Views::View
{
private:
	enum {
		offFlag,
		insideFlag,
		downFlag,
		focusFlag,
	};
	bool fMainFocus, fPointerFocus, fTrack;
	uint32_t fBtns;
	uint32_t fFlags;
	ArrayDeleter<char> fLabel;
	ObjectDeleter<Action> fAction;

	void SetFlags(uint32_t flags);

public:
	Button(const char *label, Action *action);
	~Button();
	
	void Enable(bool doEnable);

	void Draw(RasBufOfs32 rb, Rect dirty) override;
	void HandleMsg(Views::Message &_msg) override;
};


#endif	// _BUTTONS_H_
