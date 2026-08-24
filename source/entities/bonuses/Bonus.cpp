#include "entities/bonuses/Bonus.h"
#include "geometry/Point.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObjProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "utils/Log.h"

Bonus::Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, const milliseconds lifeTime,
			 std::string name, const Uuid uuid, const GameMode gameMode, const BonusType bonusType)
	: BaseObj{BaseObjProperty{.rect = rect,
							  .health = 1,
							  .uuid = uuid,
							  .name = std::move(name),
							  .fraction = "Neutral"},
			  kCollision}
	, _lifeTimeTimer{lifeTime}
	, _gameMode{gameMode}
	, _bonusType{bonusType}
	, _events{events}
{
	Subscribe();

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(
				BonusSpawnedEvent{.pos = FPoint{.x = rect.x, .y = rect.y}, .type = _bonusType, .uuid = uuid});
	}
}

Bonus::~Bonus()
{
	if (BaseObj::GetIsAlive() || !IsHost(_gameMode))
	{
		return;//NOTE: skip in case of shutdown\restart
	}

	if (_despawnReason == DespawnReason::None)
	{
		Log::Error("Bonus " + _name + " left the field without naming a reason");
	}

	_events->EmitEvent(DespawnedEvent{.who = _name, .uuid = _uuid, .reason = _despawnReason});
}

void Bonus::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Bonus::OnDraw));

	IsAuthority(_gameMode) ? SubscribeAsAuthority() : SubscribeAsClient();
}

void Bonus::OnDraw(const DrawEvent&) const { Draw(); }

void Bonus::SubscribeAsAuthority()
{
	_subs.push_back(_events->AddListener(this, &Bonus::OnTickUpdate));
}

void Bonus::OnTickUpdate(const TickUpdateEvent& event) { TickUpdate(event.deltaTime); }

void Bonus::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Bonus::OnDespawned));
}

void Bonus::OnDespawned(const DespawnedEvent&) { SetIsAlive(false); }

void Bonus::Draw() const
{
	_events->EmitEvent(DrawObjEvent{.rect = _rect, .dir = Direction::UP, .name = _name});
}

void Bonus::TickUpdate(double /*deltaTime*/)
{
	if (_lifeTimeTimer.isActive && _lifeTimeTimer.IsCooldownFinish())
	{
		_lifeTimeTimer.isActive = false;
		_despawnReason = DespawnReason::Expired;
		SetIsAlive(false);

		_events->EmitEvent(StatisticsBonusExpiredEvent{});
	}
}

void Bonus::TakeDamage(const unsigned int damage, const std::string& author, const std::string& fraction)
{
	_despawnReason = DespawnReason::Destroyed;

	BaseObj::TakeDamage(damage, author, fraction);
}

void Bonus::EmitDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(StatisticsBonusDestroyedEvent{.author = author, .fraction = fraction});
}

void Bonus::PickUpBonus(const std::string& author, const std::string& fraction)
{
	if (GetIsAlive())
	{
		_despawnReason = DespawnReason::PickedUp;

		_events->EmitEvent(StatisticsBonusPickupEvent{.author = author, .fraction = fraction});

		EmitPickupEvent(author, fraction);

		SetIsAlive(false);
	}
}
