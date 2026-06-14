#include "entities/obstacles/Obstacle.h"
#include "components/EventSystem.h"
#include "entities/BaseObjProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"

Obstacle::Obstacle(const ObjRectangle rect, const int health, std::string name,
				   const std::shared_ptr<EventSystem>& events, const buuid uuid, const GameMode gameMode,
				   const ObstacleType obstacleType)
	: BaseObj{BaseObjProperty{.rect = rect,
							  .health = health,
							  .uuid = uuid,
							  .name = std::move(name),
							  .fraction = "Neutral"}}
	, _events(events)
	, _gameMode{gameMode}
	, _obstacleType(obstacleType)
{
	Obstacle::Subscribe();

	if (_gameMode == GameMode::PlayAsHost)
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
	if (_gameMode == GameMode::PlayAsClient)
	{
		Obstacle::SubscribeAsClient();
	}
}

void Obstacle::SubscribeAsClient()
{
	_events->AddListener("ClientReceived_" + _nameWithUuid + "Health", _nameWithUuid, [this](const int health)
	{
		this->SetHealth(health);
	});
}

void Obstacle::Unsubscribe() const { _events->RemoveAllListeners(_nameWithUuid); }

void Obstacle::Draw() const { _events->EmitEvent("DrawObj", _rect, Direction::UP, _name); }

void Obstacle::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (const auto health = GetHealth();
		health < 1)
	{
		_events->EmitEvent("Statistics_" + _name + "Died", author, fraction);

		//TODO: move this to onHealthChange
		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_Health", _name, health, _uuid);
		}
	}
}
