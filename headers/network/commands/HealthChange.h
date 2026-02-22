#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>

namespace network::commands
{
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
	HealthChange(std::string who, int health, buuid uuid);

	~HealthChange() override = default;

	[[nodiscard]] std::string GetWho() const noexcept;
	[[nodiscard]] int GetHealth() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void HealthChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _who;
	ar & _health;
	ar & _uuid;
}
}//namespace network::commands

BOOST_CLASS_EXPORT_KEY(network::commands::HealthChange);
