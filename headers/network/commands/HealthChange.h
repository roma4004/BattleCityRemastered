#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include "../commands/Command.h"

class HealthChange : public Command
{
	friend class boost::serialization::access;

	std::string _who{};
	int _health{};

public:
	//for deserialization
	HealthChange();

	//for serialization
	HealthChange(const std::string& who, int health);

	~HealthChange() override = default;

	const std::string& GetWho() const;
	int GetHealth() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _health;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(HealthChange);
