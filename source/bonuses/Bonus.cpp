#include "../../headers/bonuses/Bonus.h"
#include "../../headers/utils/TimeUtils.h"

Bonus::Bonus(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
             const int durationMs, const int lifeTimeMs, const int color, const int id)
	: BaseObj{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1}, color, 1},
	  _window{std::move(window)},
	  _creationTime{std::chrono::system_clock::now()},
	  _id{id},
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

int Bonus::GetId() const { return _id; }
