#include "../../headers/pawns/Pawn.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/interfaces/IMoveBeh.h"
#include "../../headers/pawns/PawnProperty.h"

// #include <iostream>

Pawn::Pawn(PawnProperty pawnProperty, std::unique_ptr<IMoveBeh> moveBeh)
	: BaseObj{std::move(pawnProperty.baseObjProperty)},
	  _dir{pawnProperty.dir},
	  _gameMode{pawnProperty.gameMode},
	  _speed{pawnProperty.speed},
	  _tier{pawnProperty.tier},
	  _windowSize{pawnProperty.windowSize},
	  _allObjects{pawnProperty.allObjects},
	  _events{std::move(pawnProperty.events)},
	  _moveBeh{std::move(moveBeh)}
{
	Pawn::Subscribe();
}

Pawn::~Pawn()
{
	// std::cout << "[" << "Pawn::~Pawn()" << "] "
	// 			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 			<< ", name=" << _name
	// 			<< ", name+UUID=" << _nameWithUuid
	// 			<< std::endl;
	Pawn::Unsubscribe();
}

void Pawn::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });

	_gameMode == PlayAsClient ? Pawn::SubscribeAsClient() : Pawn::SubscribeAsHost();
}

void Pawn::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _nameWithUuid, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void Pawn::SubscribeAsClient()
{
	_events->AddListener<const FPoint, const Direction, const buuid&>(
			"ClientReceived_" + _name + "Pos", _nameWithUuid,
			[this](const FPoint newPos, const Direction dir, const buuid& uuid)
			{
				if (uuid != this->_uuid)
				{
					return;
				}

				this->SetDirection(dir);
				this->SetPos(newPos);
				this->UpdateAnimationFrame();
			});

	_events->AddListener<const int>(
			"ClientReceived_" + _nameWithUuid + "Health", _nameWithUuid,
			[this](const int health)
			{
				this->SetHealth(health);
			});
}

void Pawn::Unsubscribe() const
{
	// std::cout << "[" << "Pawn::Unsubscribe()" << "] "
	// 			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 			<< ", name=" << _name
	// 			<< ", name+UUID=" << _nameWithUuid
	// 			<< std::endl;
	_events->RemoveListener("Draw", _nameWithUuid);

	_gameMode == PlayAsClient ? Pawn::UnsubscribeAsClient() : Pawn::UnsubscribeAsHost();
}

void Pawn::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _nameWithUuid);
}

void Pawn::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const Direction, const buuid&>(
			"ClientReceived_" + _name + "Pos", _nameWithUuid);
	_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _nameWithUuid);
}

void Pawn::SetHealth(const int health)
{
	BaseObj::SetHealth(health);

	// if (!GetIsAlive())
	// {
	// 	Unsubscribe();
	// }
}

void Pawn::TakeDamage(const int damage)
{
	BaseObj::TakeDamage(damage);

	// if (!GetIsAlive())
	// {
	// 	Unsubscribe();
	// }
}

void Pawn::UpdateAnimationFrame()
{
	++_animationFrameId;
	if (_animationFrameId % 12 && ++_animationId > _animationIdLimit) //TODO: 12 is frame cycle, need skip for bullet
	{
		_animationId = 0;
		_animationFrameId = 0;
	}
}

void Pawn::Draw(const BaseObj* obj) const { _events->EmitEvent<const BaseObj*>("DrawObj", obj); }

UPoint Pawn::GetWindowSize() const { return _windowSize; }

Direction Pawn::GetDirection() const { return _dir; }

void Pawn::SetDirection(const Direction dir) { _dir = dir; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }
