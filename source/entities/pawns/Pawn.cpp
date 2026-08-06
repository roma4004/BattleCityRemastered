#include "entities/pawns/Pawn.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/ReplicationEvents.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h" //NOTE: required for std::unique_ptr<IMoveBeh> Pawn::_moveBeh
#include "utils/UuidUtils.h"
// #include <iostream>

Pawn::Pawn(PawnProperty pawnProperty, GameConfig& gameConfig)
	: BaseObj{std::move(pawnProperty.baseObjProperty)}
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
	_events->AddListener("ClientReceived_Health", _uuid, _nameWithUuid, [this](const int health)
	{
		this->SetHealth(health);
	});
}

void Pawn::SubscribeTickUpdate()
{
	_events->AddListener("TickUpdate", _nameWithUuid, [this](const double deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void Pawn::UnsubscribeTickUpdate() const { _events->RemoveListener("TickUpdate", _nameWithUuid); }

void Pawn::Unsubscribe() const { _events->RemoveAllListeners(_nameWithUuid); }

void Pawn::TakeDamage(const unsigned int damage, const std::string& damageAuthor, const std::string& damageFraction)
{
	BaseObj::TakeDamage(damage, damageAuthor, damageFraction);

	SendDamageStatistics(damageAuthor, damageFraction);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_Health",
						   ServerSendHealthEvent{.who = _name, .health = GetHealth(), .uuid = _uuid});
	}
}

Direction Pawn::GetDirection() const { return _dir; }

void Pawn::SetDirection(const Direction dir) { _dir = dir; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }
