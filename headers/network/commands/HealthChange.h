#pragma once

#include "UuidSerialization.h"
#include "enums/CommandType.h"
#include <ser20/types/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>

namespace network::commands
{
class HealthChange
{
	using buuid = boost::uuids::uuid;

	CommandType _type{CommandType::HEALTH_CHANGE};
	std::string _who{};
	int _health{};
	buuid _uuid{};

public:
	//for deserialization
	HealthChange() = default;

	//for serialization
	HealthChange(std::string who, int health, buuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] std::string GetWho() const noexcept;
	[[nodiscard]] int GetHealth() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void HealthChange::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _who;
	ar & _health;
	ar & _uuid;
}
}//namespace network::commands
