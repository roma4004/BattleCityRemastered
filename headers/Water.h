#pragma once

#include "BaseObj.h"
#include "EventSystem.h"

class Water final : public BaseObj
{
public:
	Water(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events);

	~Water() override;
	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	UPoint _windowSize{0, 0};

protected:
	int* _windowBuffer{nullptr};
	std::shared_ptr<EventSystem> _events;
};
