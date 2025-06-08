#pragma once

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"
#include "../../Point.h"
#include "../../enums/BonusType.h"

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
	BonusSpawn(const FPoint& pos, BonusType bonusType, buuid uuid);

	~BonusSpawn() override = default;

	FPoint GetPos() const;
	BonusType GetBonusType() const;
	buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _pos;
		ar & _bonusType;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(BonusSpawn);
