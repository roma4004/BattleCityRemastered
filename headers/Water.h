#pragma once

#include "BaseObj.h"
#include "EventSystem.h"

class Water final : public BaseObj
{
public:
	Water(const Rectangle& rect, int* windowBuffer, size_t windowWidth, size_t windowHeight,
		  std::shared_ptr<EventSystem> events);

	~Water() override;
	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	size_t _windowWidth{0};
	size_t _windowHeight{0};

protected:
	int* _windowBuffer{nullptr};
	std::shared_ptr<EventSystem> _events;
};
