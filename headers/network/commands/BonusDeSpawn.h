#pragma once

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include "../../Point.h"
#include "../commands/Command.h"

class BonusDeSpawn : public Command
{
	friend class boost::serialization::access;

	int _id{};

public:
	//for deserialization
	BonusDeSpawn();

	//for serialization
	explicit BonusDeSpawn(int id);

	~BonusDeSpawn() override = default;

	int GetId() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _id;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(BonusDeSpawn);
