#include "Components/RightSideBar.h"
#include "Components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"

RightSideBar::RightSideBar(const std::shared_ptr<EventSystem>& events)
	: _name{std::string("RightSideBar")}
	, _events{events}
{
	Subscribe();
}

void RightSideBar::Subscribe()
{
	_subs.push_back(_events->AddListener(_name, [this](const GameModeChangedToEvent& event)
	{
		this->_gameMode = event.mode;
	}));

	_subs.push_back(_events->AddListener(_name, [this](const DrawUserInterfaceEvent&) { this->Draw(); }));

	_subs.push_back(_events->AddListener(_name, [this](const RespawnCountChangedToEvent& event)
	{
		OnRespawnCountChangedTo(event.objectName, event.respawnCount);
	}));
}

void RightSideBar::Draw() const
{
	_events->EmitEvent(RenderRightSideBarEvent{});
	_events->EmitEvent(RenderEnemyIconBackgroundEvent{});
	_events->EmitEvent(RenderEnemyIconsEvent{.count = _enemiesRespawnCount});
	_events->EmitEvent(RenderPlayerOneIconEvent{.respawnCount = _playerOneRespawnCount});
	if (_gameMode != GameMode::OnePlayer)
	{
		_events->EmitEvent(RenderPlayerTwoIconEvent{.respawnCount = _playerTwoRespawnCount});
	}
	_events->EmitEvent(RenderStageNumberEvent{.stageNumber = _stageNumber});
}

void RightSideBar::OnRespawnCountChangedTo(const std::string& objectName, const unsigned short respawnCount)
{
	if (objectName == "Enemy")
	{
		_enemiesRespawnCount = respawnCount;
	}
	else if (objectName.ends_with("1"))
	{
		_playerOneRespawnCount = respawnCount;
	}
	else if (objectName.ends_with("2"))
	{
		_playerTwoRespawnCount = respawnCount;
	}
}
