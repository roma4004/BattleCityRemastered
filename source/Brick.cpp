#include "../headers/Brick.h"

#include <string>

Brick::Brick(const Rectangle& rect, int* windowBuffer, const UPoint windowSize, std::shared_ptr<EventSystem> events)
	: BaseObj{{rect.x, rect.y, rect.w - 1, rect.h - 1}, 0x924b00, 15},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	_name = "Brick " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	Subscribe();
}

Brick::~Brick()
{
	Unsubscribe();
}

void Brick::Subscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void Brick::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);
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
	std::string authorAndFractionTag = author + fraction;
	_events->EmitEvent<std::string>("BrickDied", authorAndFractionTag);
}
