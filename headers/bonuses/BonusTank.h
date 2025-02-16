#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusTank final : public Bonus
{
	void Subscribe();
	void Unsubscribe() const;

public:
	BonusTank(const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
	          std::shared_ptr<EventSystem> events, int durationMs, int lifeTimeMs, int color);

	~BonusTank() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
