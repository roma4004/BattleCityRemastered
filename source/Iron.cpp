﻿#include "../headers/Iron.h"

#include <string>

Iron::Iron(const FPoint& pos, const float width, const float height, int* windowBuffer, size_t windowWidth,
		   size_t windowHeight, std::shared_ptr<EventSystem> events)
	: BaseObj(pos, width - 1, height - 1, 0xaaaaaa, 0, 15), _windowWidth(windowWidth), _windowHeight(windowHeight),
	  _windowBuffer{windowBuffer}, _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_events == nullptr)
	{
		return;
	}

	const auto eventName =
			"Iron " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_events->AddListener("Draw", eventName, [this]() { this->Draw(); });
}

Iron::~Iron()
{
	// unsubscribe
	if (_events == nullptr)
	{
		return;
	}

	const auto eventName =
			"Iron " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_events->RemoveListener("Draw", eventName);
}

void Iron::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _windowWidth && y < _windowHeight)
	{
		const size_t rowSize = _windowWidth;
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

void Iron::TickUpdate(float deltaTime) {}
