#pragma once

#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/CommandType.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct BonusStatus final
{
	CommandType type{CommandType::BONUS_STATUS};
	std::string name{};
	Author author{};
	BonusType bonusType{};
	bool isEnable{};
};
}//namespace network::commands
