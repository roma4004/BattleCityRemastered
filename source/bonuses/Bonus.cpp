#include "../../headers/bonuses/Bonus.h"
#include "../../headers/utils/TimeUtils.h"

Bonus::Bonus(const ObjRectangle& rect, int* windowBuffer, const UPoint windowSize, std::shared_ptr<EventSystem> events,
             const int durationMs, const int lifeTimeMs, const int color)
	: BaseObj{{rect.x, rect.y, rect.w - 1, rect.h - 1}, color, 1},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _creationTime{std::chrono::system_clock::now()},
	  _bonusDurationMs{durationMs},
	  _bonusLifetimeMs{lifeTimeMs},
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

void Bonus::TickUpdate(float /*deltaTime*/)
{
	if (TimeUtils::IsCooldownFinish(_creationTime, _bonusLifetimeMs))
	{
		SetIsAlive(false);
	}
}
