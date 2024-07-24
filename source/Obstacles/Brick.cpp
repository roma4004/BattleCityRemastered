#include "../../headers/obstacles/Brick.h"

#include <string>

Brick::Brick(const ObjRectangle& rect, int* windowBuffer, const UPoint windowSize, std::shared_ptr<EventSystem> events,
             const int obstacleId)
	: BaseObj{{rect.x, rect.y, rect.w - 1, rect.h - 1}, 0x924b00, 15},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	_name = "Brick" + std::to_string(obstacleId);
	Subscribe();
}

Brick::~Brick()
{
	Unsubscribe();
}

void Brick::Subscribe()
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

void Brick::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);

	_events->RemoveListener<const int>("Net_" + _name + "_NewHealth", _name);
}

void Brick::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _windowSize.x && y < _windowSize.y)
	{
		const size_t rowSize = _windowSize.x;
		_windowBuffer[y * rowSize + x] = color;
	}
}

void Brick::Draw() const
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

void Brick::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("BrickDied", author, fraction);

		_events->EmitEvent<const std::string, const std::string, const int>(
				"SendHealth", GetName(), "_NewHealth", GetHealth());
	}
}

std::string Brick::GetName() const { return _name; }
