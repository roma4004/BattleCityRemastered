#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/EventSystem.h"

#include <string>

WaterTile::WaterTile(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const int id)
	: BaseObj{rect, 0x1e90ff, 1},
	  _window{std::move(window)},
	  _events{std::move(events)},
	  _name{"Water " + std::to_string(id)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	Subscribe();
}

WaterTile::~WaterTile()
{
	Unsubscribe();
}

void WaterTile::Subscribe() const
{
	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void WaterTile::Unsubscribe() const
{
	_events->RemoveListener("Draw", _name);
}

void WaterTile::Draw() const
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

void WaterTile::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}

std::string WaterTile::GetName() const { return _name; }

int WaterTile::GetId() const { return _id; }
