#include "../headers/Bonus.h"
#include "../headers/TimeUtils.h"

Bonus::Bonus(const Rectangle& rect, int* windowBuffer, const UPoint windowSize, std::shared_ptr<EventSystem> events,
             const int durationSec, const int lifeTimeSec, const int color)
	: BaseObj{{rect.x, rect.y, rect.w - 1, rect.h - 1}, color, 1},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _creationTime{std::chrono::system_clock::now()},
	  _bonusDurationSec{durationSec},
	  _bonusLifetimeSec{lifeTimeSec},
	  _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

Bonus::~Bonus() = default;

void Bonus::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _windowSize.x && y < _windowSize.y)
	{
		const size_t rowSize = _windowSize.x;
		_windowBuffer[y * rowSize + x] = color;
	}
}

void Bonus::Draw() const
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

void Bonus::TickUpdate(float deltaTime)
{
	if (TimeUtils::IsCooldownFinish(_creationTime, _bonusLifetimeSec))
	{
		SetIsAlive(false);
	}
}
