#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/EventSystem.h"

#include <string>

BrickWall::BrickWall(const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
             std::shared_ptr<EventSystem> events, const int obstacleId)
	: BaseObj{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1}, 0x924b00, 1},
	  _windowSize{std::move(windowSize)},
	  _windowBuffer{std::move(windowBuffer)},
	  _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	_name = "BrickWall" + std::to_string(obstacleId);//TODO: change name for statistics
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

	_events->AddListener<const int>("Net_" + _name + "_NewHealth", _name, [this](const int health)
	{
		this->SetHealth(health);
		if (GetHealth() < 1)
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

	_events->RemoveListener<const int>("Net_" + _name + "_NewHealth", _name);
}

void BrickWall::SetPixel(const size_t x, const size_t y, const int color) const
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
		_events->EmitEvent<const std::string&, const std::string&>("BrickWallDied", author, fraction);

		_events->EmitEvent<const std::string, const std::string, const int>(
				"SendHealth", GetName(), "_NewHealth", GetHealth());
	}
}

std::string BrickWall::GetName() const { return _name; }
