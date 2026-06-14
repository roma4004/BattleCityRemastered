#include "Components/RightSideBar.h"
#include "Components/EventSystem.h"
#include "components/managers/RenderManager.h"

RightSideBar::RightSideBar(const UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _windowSize{windowSize}
	, _name{std::string("RightSideBar")}
	, _events{events}
{
	Subscribe();
}

RightSideBar::~RightSideBar()
{
	Unsubscribe();
}

void RightSideBar::Subscribe()
{
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
	});

	_events->AddListener("DrawUserInterface", _name, [this]() { this->Draw(); });

	_events->AddListener(
			"RespawnCountChangedTo", _name,
			[this](const std::string& objectName, const unsigned short respawnCount)
			{
				OnRespawnCountChangedTo(objectName, respawnCount);
			});
}

void RightSideBar::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void RightSideBar::Draw() const
{
	_events->EmitEvent("RenderRightSideBar");
	_events->EmitEvent("RenderEnemyIcons", _enemiesRespawnCount);
	_events->EmitEvent("RenderStageNumber", _stageNumber);
	_events->EmitEvent("RenderPlayerOneIcon", _playerOneRespawnCount);
	if (_gameMode != GameMode::OnePlayer)
	{
		_events->EmitEvent("RenderPlayerTwoIcon", _playerTwoRespawnCount);
	}
}

void RightSideBar::OnRespawnCountChangedTo(const std::string& objectName, const unsigned short respawnCount)
{
	if (objectName.starts_with("Enemy"))
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
