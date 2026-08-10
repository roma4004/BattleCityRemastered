#include "entities/pawns/Pawn.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h" //NOTE: required for std::unique_ptr<IMoveBeh> Pawn::_moveBeh
#include "utils/UuidUtils.h"
// #include <iostream>

Pawn::Pawn(PawnProperty pawnProperty, GameConfig& gameConfig, const CollisionTags collision)
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

void Pawn::Subscribe()
{
	_gameMode == GameMode::PlayAsClient ? Pawn::SubscribeAsClient() : Pawn::SubscribeAsHost();
}

void Pawn::SubscribeAsHost() { SubscribeTickUpdate(); }

void Pawn::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(_uuid, _nameWithUuid, [this](const ClientInHealthEvent& event)
	{
		this->SetHealth(event.health);
	}));
}

void Pawn::SubscribeTickUpdate()
{
	//NOTE: guarded - Bullet::Enable() re-subscribes on pool reuse while already subscribed.
	if (!_tickUpdateSub)
	{
		_tickUpdateSub = _events->AddListener(_nameWithUuid, [this](const TickUpdateEvent& event)
		{
			this->TickUpdate(event.deltaTime);
		});
	}
}

void Pawn::UnsubscribeTickUpdate() const { _tickUpdateSub = EventSubscription{}; }

void Pawn::Unsubscribe() const
{
	_subs.clear();
	_tickUpdateSub = EventSubscription{};
}

void Pawn::TakeDamage(const unsigned int damage, const std::string& damageAuthor, const std::string& damageFraction)
{
	BaseObj::TakeDamage(damage, damageAuthor, damageFraction);

	SendDamageStatistics(damageAuthor, damageFraction);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutHealthEvent{.who = _name, .health = GetHealth(), .uuid = _uuid});
	}
}

Direction Pawn::GetDirection() const { return _dir; }

void Pawn::SetDirection(const Direction dir) { _dir = dir; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }
