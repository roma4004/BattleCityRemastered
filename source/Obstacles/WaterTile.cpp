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
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void WaterTile::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);
}

void WaterTile::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (_window->buffer == nullptr)//TODO: remove this kind of check from project, all checks covered on config create
	{
		return;
	}

	if (x < _window->size.x && y < _window->size.y)
	{
		const size_t rowSize = _window->size.x;
		_window->buffer.get()[y * rowSize + x] = color;
	}
}

void WaterTile::Draw() const
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

void WaterTile::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}

std::string WaterTile::GetName() const { return _name; }

int WaterTile::GetId() const { return _id; }
