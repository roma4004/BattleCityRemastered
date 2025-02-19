#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/EventSystem.h"

#include <string>

SteelWall::SteelWall(const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
           std::shared_ptr<EventSystem> events, const int obstacleId)
	: BaseObj{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1}, 0xaaaaaa, 1},
	  _windowSize{std::move(windowSize)},
	  _windowBuffer{std::move(windowBuffer)},
	  _events{std::move(events)},
	 _id{obstacleId}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);

	_name = "SteelWall " + std::to_string(obstacleId);//TODO: change name for statistics
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
	if (_windowBuffer == nullptr)
	{
		return;
	}

	if (x < _windowSize.x && y < _windowSize.y)
	{
		const size_t rowSize = _windowSize.x;
		_windowBuffer.get()[y * rowSize + x] = color;
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
