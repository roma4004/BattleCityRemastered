#pragma once

#include "enums/BonusType.h"
#include "enums/CommandType.h"
#include <ser20/types/common.hpp>
#include <ser20/types/string.hpp>
#include <string>

namespace network::commands
{
class BonusStatus
{
	CommandType _type{CommandType::BONUS_STATUS};
	std::string _name{};
	BonusType _bonusType{};
	bool _isEnable{};

public:
	//for deserialization
	BonusStatus() = default;

	//for serialization
	BonusStatus(std::string name, BonusType bonusType, bool isEnable);
	BonusStatus(std::string name, BonusType bonusType);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] std::string GetName() const noexcept;
	[[nodiscard]] BonusType GetBonusType() const noexcept;
	[[nodiscard]] bool GetIsEnable() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void BonusStatus::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _name;
	ar & _bonusType;
	ar & _isEnable;
}
}//namespace network::commands
