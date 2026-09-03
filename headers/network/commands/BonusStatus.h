#pragma once

#include "enums/Author.h"
#include "enums/BonusType.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct BonusStatus final
{
	std::string name{};
	Author author{};
	BonusType bonusType{};
	bool isEnable{};
};
}//namespace network::commands
