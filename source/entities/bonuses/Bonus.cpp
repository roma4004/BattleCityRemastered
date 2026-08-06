#include "entities/bonuses/Bonus.h"
#include "Point.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "entities/BaseObjProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "utils/TimeUtils.h"

Bonus::Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, const milliseconds lifeTime,
			 std::string name, const buuid uuid, const GameMode gameMode, const BonusType bonusType)
	: BaseObj{BaseObjProperty{.rect = rect,
							  .health = 1,
							  .uuid = uuid,
							  .name = std::move(name),
							  .fraction = "Neutral"}}
	, _lifeTimeTimer{lifeTime, std::chrono::system_clock::now()}
	, _gameMode{gameMode}
	, _bonusType{bonusType}
	, _events{events}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_BonusSpawn",
						   BonusSpawnEvent{.pos = FPoint{.x = rect.x, .y = rect.y}, .type = _bonusType, .uuid = uuid});
	}
}

Bonus::~Bonus()
{
	Unsubscribe();

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_BonusDeSpawn", _uuid);
	}
}

void Bonus::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(); });

	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void Bonus::SubscribeAsHost()
{
	_events->AddListener("TickUpdate", _nameWithUuid, [this](const double deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void Bonus::SubscribeAsClient()
{
	_events->AddListener("ClientReceived_BonusDeSpawn", _nameWithUuid, [this](const buuid& uuid)
	{
		if (uuid != this->_uuid)
		{
			return;
		}

		this->SetIsAlive(false);
	});
}

void Bonus::Unsubscribe() const { _events->RemoveAllListeners(_nameWithUuid); }

void Bonus::Draw() const
{
	_events->EmitEvent("DrawObj", DrawObjEvent{.rect = _rect, .dir = Direction::UP, .name = _name});
}

void Bonus::TickUpdate(double /*deltaTime*/)
{
	if (_lifeTimeTimer.isActive && _lifeTimeTimer.IsCooldownFinish())
	{
		_lifeTimeTimer.isActive = false;
		SetIsAlive(false);
	}
}

void Bonus::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent("Statistics_BonusDestroyed", StatisticsAttributionEvent{.author = author, .fraction = fraction});
}

void Bonus::PickUpBonus(const std::string& author, const std::string& fraction)
{
	if (GetIsAlive())
	{
		_events->EmitEvent("Statistics_BonusPickup",
						   StatisticsAttributionEvent{.author = author, .fraction = fraction});
		_events->EmitEvent(_name + "_Pickup", StatisticsAttributionEvent{.author = author, .fraction = fraction});
		TakeDamage(GetHealth(), _name, _fraction);
	}
}
