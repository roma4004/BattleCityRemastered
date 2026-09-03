#include "components/RightSideBar.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "enums/RespawnGroup.h"

RightSideBar::RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _gameConfig{gameConfig}
	, _events{events}
{
	Subscribe();
}

void RightSideBar::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnDrawUserInterface));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnRespawnCountChangedTo));
}

void RightSideBar::OnDrawUserInterface(const DrawUserInterfaceEvent&) const { Draw(); }

void RightSideBar::Draw() const
{
	_events->EmitEvent(RenderEnemyIconBackgroundEvent{});
	_events->EmitEvent(RenderEnemyIconsEvent{.count = _enemiesRespawnCount});
	_events->EmitEvent(RenderPlayerOneIconEvent{.respawnCount = _playerOneRespawnCount});
	if (_gameConfig.HasSecondPlayer())
	{
		_events->EmitEvent(RenderPlayerTwoIconEvent{.respawnCount = _playerTwoRespawnCount});
	}
	_events->EmitEvent(RenderStageNumberEvent{.stageNumber = _stageNumber});
}

void RightSideBar::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	switch (event.group)
	{
		case RespawnGroup::ENEMY_ALL:
			_enemiesRespawnCount = event.respawnCount;
			return;
		case RespawnGroup::PLAYER1:
			_playerOneRespawnCount = event.respawnCount;
			return;
		case RespawnGroup::PLAYER2:
			_playerTwoRespawnCount = event.respawnCount;
			return;
	}
}
