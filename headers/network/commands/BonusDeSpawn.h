#pragma once

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"

class BonusDeSpawn : public Command
{
	friend class boost::serialization::access;

	boost::uuids::uuid _uuid{};

public:
	//for deserialization
	BonusDeSpawn();

	//for serialization
	explicit BonusDeSpawn(boost::uuids::uuid uuid);

	~BonusDeSpawn() override = default;

	boost::uuids::uuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(BonusDeSpawn);
