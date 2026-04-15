#include "entities/pawns/Pawn.h"
#include "components/EventSystem.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h"
#include "utils/UuidUtils.h"
// #include <iostream>

Pawn::Pawn(PawnProperty pawnProperty)
	: BaseObj{std::move(pawnProperty.baseObjProperty)}
	, _speed{pawnProperty.speed}
	, _tier{pawnProperty.tier}
	, _windowSize{pawnProperty.windowSize}
	, _allObjects{pawnProperty.allObjects}
	, _events{std::move(pawnProperty.events)}
	, _dir{pawnProperty.dir}
	, _gameMode{pawnProperty.gameMode}
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
	_events->AddListener(
			"ClientReceived_" + _name + "Pos",
			_nameWithUuid,
			[this](const FPoint newPos, const Direction dir, const buuid& uuid)
			{
				OnClientChangePos(newPos, dir, uuid);
			});

	_events->AddListener("ClientReceived_" + _nameWithUuid + "Health", _nameWithUuid, [this](const int health)
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

void Pawn::Unsubscribe() const
{
	// std::cout << "[" << "Pawn::Unsubscribe()" << "] "
	// 			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 			<< ", name=" << _name
	// 			<< ", name+UUID=" << _nameWithUuid
	// 			<< std::endl;

	_gameMode == GameMode::PlayAsClient ? Pawn::UnsubscribeAsClient() : Pawn::UnsubscribeAsHost();
}

void Pawn::UnsubscribeAsHost() const
{
	UnsubscribeTickUpdate();
}

void Pawn::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_" + _name + "Pos", _nameWithUuid);
	_events->RemoveListener("ClientReceived_" + _nameWithUuid + "Health", _nameWithUuid);
}

void Pawn::TakeDamage(const int damage)
{
	BaseObj::TakeDamage(damage);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_Health", _name, GetHealth(), _uuid);
	}
}

UPoint Pawn::GetWindowSize() const { return _windowSize; }

Direction Pawn::GetDirection() const { return _dir; }

void Pawn::SetDirection(const Direction dir) { _dir = dir; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }

bool Pawn::Move(const double deltaTime)
{
	const bool isMove = _moveBeh->Move(deltaTime);
	if (isMove)
	{
		_events->EmitEvent("AnimationTankUpdate", std::string(GetName()), GetPos(), GetDirection());

		if (_gameMode == GameMode::PlayAsHost)// NOTE: replication position to the client
		{
			_events->EmitEvent("ServerSend_Pos", _name, GetPos(), GetDirection(), _uuid);
		}
	}

	return isMove;
}

void Pawn::OnClientChangePos(const FPoint newPos, const Direction dir, const buuid& uuid)
{
	if (uuid != _uuid)//TODO: check maybe never true
	{
		return;
	}

	SetDirection(dir);
	SetPos(newPos);

	//NOTE: fix for tank truck animation tick
	_events->EmitEvent("AnimationTankUpdate", std::string(GetName()), GetPos(), GetDirection());
}
