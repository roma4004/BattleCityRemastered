#include "entities/pawns/Pawn.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h" //NOTE: required for std::unique_ptr<IMoveBeh> Pawn::_moveBeh
#include "utils/UuidUtils.h"

Pawn::Pawn(PawnProperty pawnProperty, const GameConfig& gameConfig, const CollisionTags collision)
	: BaseObj{pawnProperty.baseObjProperty, collision}
	, _speed{pawnProperty.speed}
	, _tier{pawnProperty.tier}
	, _allObjects{pawnProperty.allObjects}
	, _events{std::move(pawnProperty.events)}
	, _dir{pawnProperty.dir}
	, _author{pawnProperty.author}
	, _gameConfig{gameConfig}
{
	if (_uuid == UuidUtils::GetNilUuid())
	{
		_uuid = UuidUtils::GetRandomUuid();
	}
}

Pawn::~Pawn() = default;

void Pawn::Activate() { Subscribe(); }

void Pawn::Deactivate() { Unsubscribe(); }

void Pawn::Subscribe()
{
	_gameConfig.IsAuthority() ? Pawn::SubscribeAsAuthority() : Pawn::SubscribeAsClient();
}

void Pawn::SubscribeAsAuthority() { SubscribeTickUpdate(); }

void Pawn::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Pawn::OnHealthChanged));
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Pawn::OnDespawned));
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Pawn::OnPosChanged));
}

//NOTE: the client runs no tick, so this is the only place a pawn moves there
void Pawn::OnPosChanged(const PosChangedEvent& event)
{
	SetDirection(event.dir);
	SetPos(event.pos);
}

void Pawn::OnDespawned(const DespawnedEvent&) { SetIsAlive(false); }

void Pawn::OnHealthChanged(const HealthChangedEvent& event) { SetHealth(event.health); }

void Pawn::SubscribeTickUpdate()
{
	//NOTE: guarded - a tank spawning with no timer bonus running is unfrozen while already ticking
	if (!_tickUpdateSub)
	{
		_tickUpdateSub = _events->AddListener(this, &Pawn::OnTickUpdate);
	}
}

void Pawn::OnTickUpdate(const TickUpdateEvent& event)
{
	//NOTE: a pawn killed earlier this tick stays subscribed until PostTickUpdate - a corpse neither moves nor shoots
	if (!GetIsAlive())
	{
		return;
	}

	TickUpdate(event.deltaTime);
}

void Pawn::UnsubscribeTickUpdate() { _tickUpdateSub = EventSubscription{}; }

void Pawn::Unsubscribe()
{
	_subs.clear();
	_tickUpdateSub = EventSubscription{};
}

Author Pawn::GetAuthor() const noexcept { return _author; }

void Pawn::TakeDamage(const unsigned int damage, const Author author)
{
	BaseObj::TakeDamage(damage, author);

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(HealthChangedEvent{.health = GetHealth(), .uuid = _uuid});
	}
}

void Pawn::Heal(const int amount)
{
	if (!_gameConfig.IsAuthority())
	{
		return;
	}

	SetHealth(GetHealth() + amount);

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(HealthChangedEvent{.health = GetHealth(), .uuid = _uuid});
	}
}

Direction Pawn::GetDirection() const noexcept { return _dir; }

void Pawn::SetDirection(const Direction dir) { _dir = dir; }

double Pawn::GetSpeed() const noexcept { return _speed; }
