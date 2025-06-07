#include "../../headers/pawns/Pawn.h"
#include "../../headers/application/Window.h"
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
	  _textureManager{pawnProperty.textureManager},
	  _allObjects{pawnProperty.allObjects},
	  _window{std::move(pawnProperty.window)},
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
	_events->AddListener("DrawTexture", _nameWithUuid, [this]()
	{
		this->Draw(this);
	});

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
	_events->AddListener<const FPoint, const Direction, const boost::uuids::uuid>(
			"ClientReceived_" + _name + "Pos", _nameWithUuid,
			[this](const FPoint newPos, const Direction dir, const boost::uuids::uuid uuid)
			{
				if (uuid != this->_uuid)
				{
					return;
				}

				this->SetDirection(dir);
				this->SetPos(newPos);
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
	_events->RemoveListener("DrawTexture", _nameWithUuid);

	_gameMode == PlayAsClient ? Pawn::UnsubscribeAsClient() : Pawn::UnsubscribeAsHost();
}

void Pawn::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _nameWithUuid);
}

void Pawn::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const Direction, const boost::uuids::uuid>(
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

void Pawn::Draw(const BaseObj* /*obj*/) const
{
	if (!GetIsAlive())
	{
		return;
	}

	if (_textureManager)
	{
		_textureManager->Draw(this);
	}
	else
	{
		int startY = static_cast<int>(GetY());
		const int startX = static_cast<int>(GetX());
		const size_t windowWidth = _window->size.x;
		const int height = static_cast<int>(GetHeight());
		const int width = static_cast<int>(GetWidth());
		const int color = GetColor();

		for (const int maxY = startY + height; startY < maxY; ++startY)
		{
			int x = startX;
			for (const int maxX = x + width; x < maxX; ++x)
			{
				const size_t offset = startY * windowWidth + startX;
				const int rowWidth = maxX - startX;
				std::ranges::fill_n(_window->buffer.get() + offset, rowWidth, color);
			}
		}
	}
}

UPoint Pawn::GetWindowSize() const { return _window->size; }

Direction Pawn::GetDirection() const { return _dir; }

void Pawn::SetDirection(const Direction dir) { _dir = dir; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }
