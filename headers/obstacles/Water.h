#pragma once

#include "../BaseObj.h"
#include "../Point.h"

#include <memory>

struct ObjRectangle;
class EventSystem;

class Water final : public BaseObj
{
	UPoint _windowSize{.x = 0, .y = 0};
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name;

	void Subscribe() const;
	void Unsubscribe() const;

	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

public:
	Water(const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
	      std::shared_ptr<EventSystem> events, int obstacleId);

	~Water() override;
};
