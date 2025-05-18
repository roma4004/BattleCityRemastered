#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"

class HealthChange : public Command
{
	friend class boost::serialization::access;

	std::string _who{};
	int _health{};
	boost::uuids::uuid _uuid{};

public:
	//for deserialization
	HealthChange();

	//for serialization
	HealthChange(const std::string& who, int health, boost::uuids::uuid uuid);

	~HealthChange() override = default;

	const std::string& GetWho() const;
	int GetHealth() const;
	boost::uuids::uuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _health;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(HealthChange);
