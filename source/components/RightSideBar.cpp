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
	_events->AddListener(
		"GameModeChangedTo", _name,
		[this](const GameMode newGameMode)
		{
			this->_gameMode = newGameMode;
		});

	_events->AddListener("RenderRightSideBar", _name, [this]()
	{
		GameMode currentGameMode = this->_gameMode;

		_events->EmitEvent("RenderEnemyIcons", _enemiesRespawnCount);
		_events->EmitEvent("RenderCurrentStageNumber", _currentStageNumber);
		_events->EmitEvent("RenderPlayerOneIcon", _playerOneRespawnCount);
		if (currentGameMode != GameMode::OnePlayer)
		{
			_events->EmitEvent("RenderPlayerTwoIcon", _playerTwoRespawnCount);
		}
	});

	_events->AddListener("RespawnCountChangedTo",_name, [this](const std::string& objectName, int respawnCount)
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
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("RenderRightSideBar", _name);
	_events->RemoveListener("RespawnCountChangedTo",_name);
}

RightSideBar::~RightSideBar()
{
	Unsubscribe();
}
