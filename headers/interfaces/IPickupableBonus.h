#pragma once

#include <string>

class IPickupableBonus
{
protected:
	virtual ~IPickupableBonus() = default;

public:
	virtual void PickUpBonus(const std::string& author, const std::string& fraction) = 0;
};
