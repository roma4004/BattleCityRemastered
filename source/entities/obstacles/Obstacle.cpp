#include "entities/obstacles/Obstacle.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

Obstacle::Obstacle(const ObjRectangle rect, const int color, const int health, std::string name,
                   std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode,
                   const ObstacleType obstacleType)
	: BaseObj{rect, color, health, uuid, std::move(name), "Neutral"},
	  _events(std::move(events)),
	  _gameMode{gameMode},
	  _obstacleType(obstacleType),
	  _isReplicationOn(true)
{
	Obstacle::Subscribe();

	if (_isReplicationOn && _gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_ObstacleSpawn", _rect, _obstacleType, uuid);
	}
}

Obstacle::Obstacle(const ObjRectangle rect, const int color, const int health, std::string name,
                   std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode,
                   const ObstacleType obstacleType, const bool isReplicationOn)
	: BaseObj{rect, color, health, uuid, std::move(name), "Neutral"},
	  _events(std::move(events)),
	  _gameMode{gameMode},
	  _obstacleType(obstacleType),
	  _isReplicationOn(isReplicationOn)
{
	Obstacle::Subscribe();

	if (_isReplicationOn && _gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_ObstacleSpawn", _rect, _obstacleType, uuid);
	}
}

Obstacle::~Obstacle()
{
	Obstacle::Unsubscribe();
}

void Obstacle::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { Draw(this); });

	if (_isReplicationOn && _gameMode == GameMode::PlayAsClient)
	{
		Obstacle::SubscribeAsClient();
	}
}

void Obstacle::SubscribeAsClient()
{
	_events->AddListener("ClientReceived_" + _nameWithUuid + "Health", _nameWithUuid, [this](const int health)
	{
		this->SetHealth(health);
		if (this->GetHealth() < 1)
		{
			this->SetIsAlive(false);
		}
	});
}

void Obstacle::Unsubscribe() const
{
	_events->RemoveListener("Draw", _nameWithUuid);

	if (_isReplicationOn && _gameMode == GameMode::PlayAsClient)
	{
		Obstacle::UnsubscribeAsClient();
	}
}

void Obstacle::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_" + _nameWithUuid + "Health", _nameWithUuid);
}

void Obstacle::Draw(const BaseObj* obj) const { _events->EmitEvent("DrawObj", obj); }

void Obstacle::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (auto health = GetHealth();
		health < 1)
	{
		_events->EmitEvent("Statistics_" + _name + "Died", author, fraction);

		//TODO: move this to onHealthChange
		if (_isReplicationOn && _gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_Health", _name, health, _uuid);
		}
	}
}
