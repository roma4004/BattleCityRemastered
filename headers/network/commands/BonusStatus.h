#pragma once

#include "enums/Author.h"
#include "enums/BonusType.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct BonusStatus final
{
	Author author{};
	BonusType bonusType{};
	bool isEnable{};
};
}//namespace network::commands
