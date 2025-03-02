#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/EventSystem.h"

#include <string>

SteelWall::SteelWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const int id)
	: BaseObj{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1}, 0xaaaaaa, 1},
	  _window{std::move(window)},
	  _events{std::move(events)},
	  _id{id}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);

	_name = "SteelWall " + std::to_string(id);//TODO: change name for statistics
	Subscribe();
}

SteelWall::~SteelWall()
{
	Unsubscribe();
}

void SteelWall::Subscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void SteelWall::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);
}

void SteelWall::SetPixel(const size_t x, const size_t y, const int color) const
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

void SteelWall::Draw() const
{
	if (!GetIsAlive())
	{
		return;
	}

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

void SteelWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("SteelWallDied", author, fraction);
	}
}

std::string SteelWall::GetName() const { return _name; }

int SteelWall::GetId() const { return _id; }
