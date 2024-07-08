#include "../../headers/obstacles/Iron.h"

#include <string>

Iron::Iron(const Rectangle& rect, int* windowBuffer, const UPoint windowSize, std::shared_ptr<EventSystem> events)
	: BaseObj{{rect.x, rect.y, rect.w - 1, rect.h - 1}, 0xaaaaaa, 15},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);

	_name = "Iron " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	Subscribe();
}

Iron::~Iron()
{
	Unsubscribe();
}

void Iron::Subscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void Iron::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);
}

void Iron::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _windowSize.x && y < _windowSize.y)
	{
		const size_t rowSize = _windowSize.x;
		_windowBuffer[y * rowSize + x] = color;
	}
}

void Iron::Draw() const
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

void Iron::SendDamageStatistics(const std::string& author, const std::string& fraction) {}
