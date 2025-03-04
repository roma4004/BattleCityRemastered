#include "../../headers/bonuses/Bonus.h"
#include "../../headers/utils/TimeUtils.h"

Bonus::Bonus(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
             const int durationMs, const int lifeTimeMs, const int color, std::string name, const int id)
	: BaseObj{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1}, color, 1, id, std::move(name)},
	  _window{std::move(window)},
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

void Bonus::Draw() const
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

void Bonus::TickUpdate(float /*deltaTime*/)
{
	if (TimeUtils::IsCooldownFinish(_creationTime, _bonusLifetimeMs))
	{
		SetIsAlive(false);
	}
}

int Bonus::GetId() const { return _id; }
