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
	: BaseObj{std::move(pawnProperty.baseObjProperty), collision}
	, _speed{pawnProperty.speed}
	, _tier{pawnProperty.tier}
	, _allObjects{pawnProperty.allObjects}
	, _events{std::move(pawnProperty.events)}
	, _dir{pawnProperty.dir}
	, _gameMode{pawnProperty.gameMode}
	, _gameConfig{gameConfig}
{
	if (_uuid == UuidUtils::GetNilUuid())
	{
		_uuid = UuidUtils::GetRandomUuid();
	}

	_uuidStr = UuidUtils::GetStringUuid(_uuid);
}

Pawn::~Pawn() = default;

void Pawn::Activate() { Subscribe(); }

void Pawn::Deactivate() { Unsubscribe(); }

void Pawn::Subscribe()
{
	IsAuthority(_gameMode) ? Pawn::SubscribeAsAuthority() : Pawn::SubscribeAsClient();
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
	//NOTE: guarded - a tank that spawns with no grenade running gets an unfreeze it never asked
	//for (Tank::OnBonusTimerReApplyOnSpawn), and it arrives while the tick is already subscribed.
	if (!_tickUpdateSub)
	{
		_tickUpdateSub = _events->AddListener(this, &Pawn::OnTickUpdate);
	}
}

void Pawn::OnTickUpdate(const TickUpdateEvent& event)
{
	//NOTE: a pawn killed earlier in this same tick keeps its subscription until PostTickUpdate,
	//so the broadcast still reaches it - a corpse neither moves nor shoots
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

void Pawn::TakeDamage(const unsigned int damage, const std::string& author, Faction faction)
{
	BaseObj::TakeDamage(damage, author, faction);

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(HealthChangedEvent{.who = _name, .health = GetHealth(), .uuid = _uuid});
	}
}

void Pawn::Heal(const int amount)
{
	if (!IsAuthority(_gameMode))
	{
		return;
	}

	SetHealth(GetHealth() + amount);

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(HealthChangedEvent{.who = _name, .health = GetHealth(), .uuid = _uuid});
	}
}

Direction Pawn::GetDirection() const { return _dir; }

void Pawn::SetDirection(const Direction dir) { _dir = dir; }

double Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const double speed) { _speed = speed; }
