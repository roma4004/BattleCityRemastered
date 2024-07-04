#pragma once

#include "BaseObj.h"
#include "EventSystem.h"
#include "Point.h"

#include <memory>

class Brick final : public BaseObj
{
	UPoint _windowSize{0, 0};
	int* _windowBuffer{nullptr};
	std::shared_ptr<EventSystem> _events;
	std::string _name;

public:
	Brick(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events);

	~Brick() override;

	void Subscribe() const;
	void Unsubscribe() const;

	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
