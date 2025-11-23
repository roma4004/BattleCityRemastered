#include "network/commands/BonusStatus.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <utility>

BOOST_CLASS_EXPORT_IMPLEMENT(BonusStatus);

BonusStatus::BonusStatus() : Command{CommandType::BONUS_STATUS} {}

BonusStatus::BonusStatus(std::string name, BonusType bonusType, bool isEnable)
	: Command{CommandType::BONUS_STATUS}, _name{std::move(name)}, _bonusType{bonusType}, _isEnable{isEnable} {}

std::string BonusStatus::GetName() const noexcept { return _name; }

BonusType BonusStatus::GetBonusType() const noexcept { return _bonusType; }

bool BonusStatus::GetIsEnable() const noexcept { return _isEnable; }

const char* BonusStatus::GetClassNameW() const noexcept { return "BonusStatus"; }
