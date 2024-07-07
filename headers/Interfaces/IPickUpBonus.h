#pragma once
#include <chrono>

class IPickUpBonus
{
protected:
	virtual ~IPickUpBonus() = default;

public:
	virtual void PickUpBonus(const std::string& author, const std::string& fraction) = 0;
};
