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
	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void SteelWall::Unsubscribe() const
{
	_events->RemoveListener("Draw", _name);
}

void SteelWall::Draw() const
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

void SteelWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("SteelWallDied", author, fraction);
	}
}

std::string SteelWall::GetName() const { return _name; }

int SteelWall::GetId() const { return _id; }
