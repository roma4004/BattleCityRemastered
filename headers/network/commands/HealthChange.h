#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class HealthChange : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	std::string _who{};
	int _health{};
	buuid _uuid{};

public:
	//for deserialization
	HealthChange();

	//for serialization
	HealthChange(const std::string& who, int health, buuid uuid);

	~HealthChange() override = default;

	const std::string& GetWho() const;
	int GetHealth() const;
	buuid GetUuid() const;

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
