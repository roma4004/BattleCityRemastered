#pragma once

#include "Command.h"
#include "Point.h"
#include "UuidSerialization.h"
#include "enums/BonusType.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class BonusSpawn : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	FPoint _pos{};
	BonusType _bonusType{};
	buuid _uuid{};

public:
	//for deserialization
	BonusSpawn();

	//for serialization
	BonusSpawn(FPoint pos, BonusType bonusType, buuid uuid);

	~BonusSpawn() override = default;

	[[nodiscard]] FPoint GetPos() const noexcept;
	[[nodiscard]] BonusType GetBonusType() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void BonusSpawn::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _pos;
	ar & _bonusType;
	ar & _uuid;
}

BOOST_CLASS_EXPORT_KEY(BonusSpawn);
