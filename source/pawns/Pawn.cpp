#include "../../headers/pawns/Pawn.h"

Pawn::Pawn(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
           const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           std::shared_ptr<EventSystem> events, std::unique_ptr<IMoveBeh> moveBeh, const int id, std::string name)
	: BaseObj{rect, color, health, id, std::move(name)},
	  _direction{direction},
	  _speed{speed},
	  _moveBeh{std::move(moveBeh)},
	  _window{std::move(window)},
	  _events{std::move(events)},
	  _allObjects{allObjects} {}

Pawn::~Pawn() = default;

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

void Pawn::SetDirection(const Direction direction) { _direction = direction; }

float Pawn::GetSpeed() const { return _speed; }

void Pawn::SetSpeed(const float speed) { _speed = speed; }
