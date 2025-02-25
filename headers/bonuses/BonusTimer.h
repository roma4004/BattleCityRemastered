#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusTimer final : public Bonus
{
	void Subscribe();
	void Unsubscribe() const;

public:
	BonusTimer(const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
	           std::shared_ptr<EventSystem> events, int durationMs, int lifeTimeMs, int color, int id);

	~BonusTimer() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
