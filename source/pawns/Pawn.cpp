#include "../../headers/pawns/Pawn.h"

Pawn::Pawn(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
           const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           std::shared_ptr<EventSystem> events, std::unique_ptr<IMoveBeh> moveBeh)
	: BaseObj{rect, color, health},
	  _direction{direction},
	  _speed{speed},
	  _moveBeh{std::move(moveBeh)},
	  _window{std::move(window)},
	  _events{std::move(events)},
	  _allObjects{allObjects} {}

Pawn::~Pawn() = default;

void Pawn::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (_window->buffer == nullptr)
	{
		return;
	}

	if (x < _window->size.x && y < _window->size.y)
	{
		const size_t rowSize = _window->size.x;
		_window->buffer.get()[y * rowSize + x] = color;
	}
}

void Pawn::Draw() const
{
	int y = static_cast<int>(GetY());
	for (const int maxY = y + static_cast<int>(GetHeight()); y < maxY; ++y)
	{
		int x = static_cast<int>(GetX());
		for (const int maxX = x + static_cast<int>(GetWidth()); x < maxX; ++x)
		{
			SetPixel(x, y, GetColor());
		}
	}
}

UPoint Pawn::GetWindowSize() const { return _window->size; }

Direction Pawn::GetDirection() const { return _direction; }

void Pawn::SetDirection(const Direction direction) { _direction = direction; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }
