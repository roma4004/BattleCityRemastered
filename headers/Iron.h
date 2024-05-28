#pragma once

#include "BaseObj.h"
#include "EventSystem.h"

struct FPoint;

class Iron final : public BaseObj
{
public:
	Iron(const FPoint& pos, float width, float height, int* windowBuffer, size_t windowWidth, size_t windowHeight,
		 std::shared_ptr<EventSystem> events);

	~Iron() override;
	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	size_t _windowWidth{};
	size_t _windowHeight{};

protected:
	int* _windowBuffer;
	std::shared_ptr<EventSystem> _events;
};
