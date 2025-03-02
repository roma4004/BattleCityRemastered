#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/EventSystem.h"

#include <string>

BrickWall::BrickWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const int id)
	: BaseObj{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1}, 0x924b00, 1},
	  _window{std::move(window)},
	  _events{std::move(events)},
	  _name{"BrickWall" + std::to_string(id)},//TODO: change name for statistics
	  _id{id}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

BrickWall::~BrickWall()
{
	Unsubscribe();
}

void BrickWall::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	//TODO: skip if in host mode
	_events->AddListener<const int>("ClientReceived_" + _name + "Health", _name, [this](const int health)
	{
		this->SetHealth(health);
		if (this->GetHealth() < 1)
		{
			this->SetIsAlive(false);
		}
	});
}

void BrickWall::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);

	_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _name);
}

void BrickWall::SetPixel(const size_t x, const size_t y, const int color) const
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

void BrickWall::Draw() const
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

void BrickWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("BrickWallDied", author, fraction);//for statistic

		//TODO: move this to onHealthChange
		_events->EmitEvent<const std::string, const int>("ServerSend_Health", GetName(), GetHealth());
		//for net replication
	}
}

std::string BrickWall::GetName() const { return _name; }

int BrickWall::GetId() const { return _id; }
