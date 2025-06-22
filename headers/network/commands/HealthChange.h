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

	[[nodiscard]] const std::string& GetWho() const;
	[[nodiscard]] int GetHealth() const;
	[[nodiscard]] buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const override;
};

template<class Archive>
void HealthChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _who;
	ar & _health;
	ar & _uuid;
}

BOOST_CLASS_EXPORT_KEY(HealthChange);
