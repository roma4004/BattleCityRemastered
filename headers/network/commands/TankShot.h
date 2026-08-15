#pragma once

#include "UuidSerialization.h"
#include "enums/CommandType.h"
#include "enums/Direction.h"
#include <ser20/types/common.hpp>
#include <ser20/types/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>

namespace network::commands
{
class TankShot
{
	using buuid = boost::uuids::uuid;

	CommandType _type{CommandType::TANK_SHOT};
	std::string _who{};
	Direction _dir{};
	buuid _uuid{};

public:
	//for deserialization
	TankShot() = default;

	//for serialization
	TankShot(std::string who, Direction dir, buuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] std::string GetWho() const noexcept;
	[[nodiscard]] Direction GetDir() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void TankShot::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _who;
	ar & _dir;
	ar & _uuid;
}
}//namespace network::commands
