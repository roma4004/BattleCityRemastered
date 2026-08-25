#pragma once

#include <string>

enum class Faction : char8_t;

class IPickupableBonus
{
protected:
	virtual ~IPickupableBonus() = default;

public:
	virtual void PickUpBonus(const std::string& author, Faction faction) = 0;
};
