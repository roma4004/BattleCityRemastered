#include "network/commands/BonusStatus.h"
#include <utility>

namespace network::commands
{
BonusStatus::BonusStatus(std::string name, BonusType bonusType, bool isEnable)
	: _name{std::move(name)}
	, _bonusType{bonusType}
	, _isEnable{isEnable} {}

BonusStatus::BonusStatus(std::string name, BonusType bonusType)
	: _name{std::move(name)}
	, _bonusType{bonusType} {}

CommandType BonusStatus::GetType() const noexcept { return _type; }

std::string BonusStatus::GetName() const noexcept { return _name; }

BonusType BonusStatus::GetBonusType() const noexcept { return _bonusType; }

bool BonusStatus::GetIsEnable() const noexcept { return _isEnable; }

const char* BonusStatus::GetClassNameW() const noexcept { return "BonusStatus"; }
}//namespace network::commands
