#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class BonusDeSpawn : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	buuid _uuid{};

public:
	//for deserialization
	BonusDeSpawn();

	//for serialization
	explicit BonusDeSpawn(buuid uuid);

	~BonusDeSpawn() override = default;

	buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(BonusDeSpawn);
