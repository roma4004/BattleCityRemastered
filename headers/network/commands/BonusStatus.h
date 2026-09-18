#pragma once

#include "enums/Author.h"
#include "enums/BonusType.h"

namespace network::commands
{
struct BonusStatus final
{
	Author author{};
	BonusType bonusType{};
	bool isEnable{};
};
}//namespace network::commands
