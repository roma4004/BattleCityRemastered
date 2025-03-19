#include "../../headers/pawns/Pawn.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/interfaces/IMoveBeh.h"

Pawn::Pawn(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
           const Direction dir, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           std::shared_ptr<EventSystem> events, std::unique_ptr<IMoveBeh> moveBeh, const int id, std::string name,
           std::string fraction, const int tier, const GameMode gameMode)
	: BaseObj{rect, color, health, id, std::move(name), std::move(fraction)},
	  _direction{dir},
	  _gameMode{gameMode},
	  _speed{speed},
	  _tier{tier},
	  _moveBeh{std::move(moveBeh)},
	  _window{std::move(window)},
	  _events{std::move(events)},
	  _allObjects{allObjects}
{
	Pawn::Subscribe();
}

Pawn::~Pawn()
{
	Pawn::Unsubscribe();
}

void Pawn::Subscribe()
{
	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_gameMode == PlayAsClient ? Pawn::SubscribeAsClient() : Pawn::SubscribeAsHost();
}

void Pawn::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void Pawn::SubscribeAsClient()
{
	_events->AddListener<const FPoint, const Direction>(
			"ClientReceived_" + _name + "Pos", _name, [this](const FPoint newPos, const Direction dir)
			{
				this->SetDirection(dir);
				this->SetPos(newPos);
			});

	_events->AddListener<const int>("ClientReceived_" + _name + "Health", _name, [this](const int health)
	{
		this->SetHealth(health);
	});
}

void Pawn::Unsubscribe() const
{
	_events->RemoveListener("Draw", _name);

	_gameMode == PlayAsClient ? Pawn::UnsubscribeAsClient() : Pawn::UnsubscribeAsHost();
}

void Pawn::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
}

void Pawn::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const Direction>("ClientReceived_" + _name + "Pos", _name);
	_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _name);
}

void Pawn::Draw() const
{
	if (!GetIsAlive())
	{
		return;
	}

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

UPoint Pawn::GetWindowSize() const { return _window->size; }

Direction Pawn::GetDirection() const { return _direction; }

void Pawn::SetDirection(const Direction dir) { _direction = dir; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }
