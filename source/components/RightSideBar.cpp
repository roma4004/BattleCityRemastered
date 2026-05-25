#include "Components/RightSideBar.h"
#include "Components/EventSystem.h"
#include "components/managers/RenderManager.h"

RightSideBar::RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _windowSize{windowSize}
	, _events{events}
	, _name{std::string("RightSideBar")}
{
	Subscribe();
}

void RightSideBar::Subscribe(){}

void RightSideBar::Unsubscribe() const{}

RightSideBar::~RightSideBar()
{
	_events->EmitEvent("DisableRightSideBar");
	Unsubscribe();
}