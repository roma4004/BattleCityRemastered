#pragma once

#include "enums/Author.h"

class IPickupableBonus
{
protected:
	virtual ~IPickupableBonus() = default;

public:
	virtual void PickUpBonus(Author author) = 0;
};
