#pragma once

#include "BaseObj.h"
#include "EventSystem.h"
#include "Point.h"

class Iron final : public BaseObj
{
public:
	Iron(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events);

	~Iron() override;

	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	UPoint _windowSize{0, 0};

protected:
	int* _windowBuffer{nullptr};
	std::shared_ptr<EventSystem> _events;
};
