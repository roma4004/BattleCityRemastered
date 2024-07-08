#pragma once

#include "BaseObj.h"
#include "EventSystem.h"
#include "Point.h"
#include "Interfaces/IPickupableBonus.h"
#include "interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>

class Bonus : public BaseObj, public ITickUpdatable, public IPickupableBonus
{
	UPoint _windowSize{0, 0};
	int* _windowBuffer{nullptr};
	std::chrono::system_clock::time_point _creationTime;

protected:
	int _bonusDurationSec{0};
	int _bonusLifetimeSec{0};

	std::string _name{};
	std::shared_ptr<EventSystem> _events;

	virtual void SetPixel(size_t x, size_t y, int color) const;

	void TickUpdate(float deltaTime) override;

	void Draw() const override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override = 0;

public:
	Bonus(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events,
	      int durationSec, int lifeTimeSec, int color);

	~Bonus() override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override = 0;
};
