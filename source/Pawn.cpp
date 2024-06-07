#include "../headers/Pawn.h"

Pawn::Pawn(const Rectangle& rect, const int color, const int health, int* windowBuffer, const UPoint windowSize,
           std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events,
           std::shared_ptr<MoveBeh> moveBeh)
	: BaseObj{rect, color, health}, _moveBeh{std::move(moveBeh)}, _windowSize{windowSize}, _windowBuffer{windowBuffer},
	  _events{std::move(events)}, _allPawns{allPawns} {}

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

void Pawn::TickUpdate(const float deltaTime) { Move(deltaTime); }
