#pragma once

#include "BaseObj.h"
#include "EventSystem.h"

class Brick final : public BaseObj
{
public:
	Brick(const Rectangle& rect, int* windowBuffer, size_t windowWidth, size_t windowHeight,
		  std::shared_ptr<EventSystem> events);

	~Brick() override;
	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	size_t _windowWidth{};
	size_t _windowHeight{};

protected:
	int* _windowBuffer;
	std::shared_ptr<EventSystem> _events;
};
