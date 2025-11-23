#pragma once

#include "Command.h"
#include "enums/BonusType.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

class BonusStatus : public Command
{
	friend class boost::serialization::access;

	std::string _name{};
	BonusType _bonusType{};
	bool _isEnable{};

public:
	//for deserialization
	BonusStatus();

	//for serialization
	BonusStatus(std::string name, BonusType bonusType, bool isEnable);

	~BonusStatus() override = default;

	[[nodiscard]] std::string GetName() const noexcept;
	[[nodiscard]] BonusType GetBonusType() const noexcept;
	[[nodiscard]] bool GetIsEnable() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void BonusStatus::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _name;
	ar & _bonusType;
	ar & _isEnable;
}

BOOST_CLASS_EXPORT_KEY(BonusStatus);
