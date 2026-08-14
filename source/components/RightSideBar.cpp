#include "Components/RightSideBar.h"
#include "Components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"

RightSideBar::RightSideBar(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

void RightSideBar::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnDrawUserInterface));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnRespawnCountChangedTo));
}

void RightSideBar::OnGameModeChangedTo(const GameModeChangedToEvent& event) { _gameMode = event.mode; }
void RightSideBar::OnDrawUserInterface(const DrawUserInterfaceEvent&) const { Draw(); }

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

void RightSideBar::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	const auto& objectName = event.objectName;
	if (objectName == "Enemy")
	{
		_enemiesRespawnCount = event.respawnCount;
	}
	else if (objectName.ends_with("1"))
	{
		_playerOneRespawnCount = event.respawnCount;
	}
	else if (objectName.ends_with("2"))
	{
		_playerTwoRespawnCount = event.respawnCount;
	}
}
