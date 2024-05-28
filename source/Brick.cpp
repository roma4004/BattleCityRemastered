#include "../headers/Brick.h"

#include <string>

Brick::Brick(const Rectangle& rect, int* windowBuffer, const UPoint windowSize, std::shared_ptr<EventSystem> events)
	: BaseObj{{rect.x, rect.y, rect.w - 1, rect.h - 1}, 0x924b00, 0, 15}, _windowSize{windowSize},
	  _windowBuffer{windowBuffer}, _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_events == nullptr)
	{
		return;
	}

	const auto name = "Brick " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_events->AddListener("Draw", name, [this]() { this->Draw(); });
}

Brick::~Brick()
{
	// unsubscribe
	if (_events == nullptr)
	{
		return;
	}

	const auto name = "Brick " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_events->RemoveListener("Draw", name);
}

void Brick::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _windowSize.x && y < _windowSize.y)
	{
		const size_t rowSize = _windowSize.x;
		_windowBuffer[y * rowSize + x] = color;
	}
}

void Brick::Draw() const
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

void Brick::TickUpdate(float deltaTime) {}
