#pragma once

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include "../../Point.h"
#include "../../enums/BonusType.h"
#include "../commands/Command.h"

class BonusSpawn : public Command
{
	friend class boost::serialization::access;

	FPoint _pos{};
	BonusType _bonusType{};
	int _id{};

public:
	//for deserialization
	BonusSpawn();

	//for serialization
	BonusSpawn(const FPoint& pos, BonusType bonusType, int id);

	~BonusSpawn() override = default;

	FPoint GetPos() const;
	BonusType GetBonusType() const;
	int GetId() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _pos;
		ar & _bonusType;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(BonusSpawn);
