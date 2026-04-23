#include "Components/RightSideBar.h"
#include "Components/EventSystem.h"

RightSideBar::RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: windowSize{windowSize}
	, _events{events}
{
	_events->EmitEvent("EnableRightSideBar");
}

void RightSideBar::CallRenderForRightSideBar() const {_events->EmitEvent("RenderRightSideBar");}

RightSideBar::~RightSideBar()
{
	_events->EmitEvent("DisableRightSideBar");
};

