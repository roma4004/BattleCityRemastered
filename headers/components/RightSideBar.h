#pragma once
#include "Point.h"
class EventSystem;

class RightSideBar
{
	Point _pos;
	UPoint windowSize;
	std::shared_ptr<EventSystem> _events{nullptr};

	void CallRenderForRightSideBar() const;

public:
	RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events);
	~RightSideBar();
};
