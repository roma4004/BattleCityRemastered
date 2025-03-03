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
	_events->RemoveListener("Draw", _name);

	_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _name);
}

void BrickWall::Draw() const //TODO: extract to new parent class obstacle
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
