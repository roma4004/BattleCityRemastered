#include "entities/bonuses/Bonus.h"
#include "Point.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/events/TimingEvents.h"
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
							  .fraction = "Neutral"},
			  kCollision}
	, _lifeTimeTimer{lifeTime, std::chrono::system_clock::now()}
	, _gameMode{gameMode}
	, _bonusType{bonusType}
	, _events{events}
{
	Subscribe();

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(
				ServerOutBonusSpawnEvent{.pos = FPoint{.x = rect.x, .y = rect.y}, .type = _bonusType, .uuid = uuid});
	}
}

Bonus::~Bonus()
{
	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutBonusDeSpawnEvent{.uuid = _uuid});
	}
}

void Bonus::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Bonus::OnDraw));

	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void Bonus::OnDraw(const DrawEvent&) { Draw(); }

void Bonus::SubscribeAsHost()
{
	_subs.push_back(_events->AddListener(this, &Bonus::OnTickUpdate));
}

void Bonus::OnTickUpdate(const TickUpdateEvent& event) { TickUpdate(event.deltaTime); }

void Bonus::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(this, &Bonus::OnClientInBonusDeSpawn));
}

void Bonus::OnClientInBonusDeSpawn(const ClientInBonusDeSpawnEvent& event)
{
	if (event.uuid != _uuid)
	{
		return;
	}

	SetIsAlive(false);
}

void Bonus::Draw() const
{
	_events->EmitEvent(DrawObjEvent{.rect = _rect, .dir = Direction::UP, .name = _name});
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
	_events->EmitEvent(StatisticsBonusDestroyedEvent{.author = author, .fraction = fraction});
}

void Bonus::PickUpBonus(const std::string& author, const std::string& fraction)
{
	if (GetIsAlive())
	{
		_events->EmitEvent(StatisticsBonusPickupEvent{.author = author, .fraction = fraction});

		EmitPickupEvent(author, fraction);

		TakeDamage(GetHealth(), _name, _fraction);
	}
}
