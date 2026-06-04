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

void RightSideBar::Subscribe()
{
	_events->AddListener("RenderRightSideBar", _name, [this]()
	{
		this->SendEnemyIconsData();
	});

	_events->AddListener("RespawnCountChangedTo",_name, [this](const std::string& objectName, const int respawnCount)
	{
		if (objectName.starts_with("Enemy"))
		{
			this->_enemiesRespawnCount = respawnCount;
		}
		else if(objectName.ends_with("1"))
		{
			this->_playerOneRespawnCount = respawnCount;
		}
		else if (objectName.ends_with("2"))
		{
			this->_playerTwoRespawnCount = respawnCount;
		}
	});
}

void RightSideBar::Unsubscribe()
{
	_events->RemoveListener("RenderRightSideBar", _name);
	_events->RemoveListener("RespawnCountChangedTo",_name);
}

void RightSideBar::SendEnemyIconsData() const
{
	_events->EmitEvent("RenderEnemyIcons", _enemiesRespawnCount);
}

ObjRectangle RightSideBar::GetRect() const
{
	const ObjRectangle rect{.x = 625, .y = 0, .w = 220, .h = 600};
	return rect;
}

RightSideBar::~RightSideBar()
{
	Unsubscribe();
}
