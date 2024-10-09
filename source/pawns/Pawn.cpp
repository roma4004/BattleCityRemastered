#include "../../headers/pawns/Pawn.h"

Pawn::Pawn(const ObjRectangle& rect, const int color, const int health, int* windowBuffer, const UPoint windowSize,
           const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           std::shared_ptr<EventSystem> events, std::shared_ptr<IMoveBeh> moveBeh)
	: BaseObj{rect, color, health},
	  _moveBeh{std::move(moveBeh)},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _direction{direction},
	  _speed{speed},
	  _events{std::move(events)},
	  _allObjects{allObjects} {}

Pawn::~Pawn() = default;

void Pawn::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _windowSize.x && y < _windowSize.y)
	{
		const size_t rowSize = _windowSize.x;
		_windowBuffer[y * rowSize + x] = color;
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

UPoint Pawn::GetWindowSize() const { return _windowSize; }

Direction Pawn::GetDirection() const { return _direction; }

void Pawn::SetDirection(const Direction direction) { _direction = direction; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }
