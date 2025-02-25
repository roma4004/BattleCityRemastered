#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/EventSystem.h"

#include <string>

WaterTile::WaterTile(const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
             std::shared_ptr<EventSystem> events, const int obstacleId)
	: BaseObj{rect, 0x1e90ff, 1},
	  _windowSize{std::move(windowSize)},
	  _windowBuffer{std::move(windowBuffer)},
	  _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	_name = "Water " + std::to_string(obstacleId);
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
