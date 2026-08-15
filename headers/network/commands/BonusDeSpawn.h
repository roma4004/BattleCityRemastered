#pragma once

#include "UuidSerialization.h"
#include "enums/CommandType.h"
#include <boost/uuid/uuid.hpp>

namespace network::commands
{
class BonusDeSpawn
{
	using buuid = boost::uuids::uuid;

	CommandType _type{CommandType::BONUS_DESPAWN};
	buuid _uuid{};

public:
	//for deserialization
	BonusDeSpawn() = default;

	//for serialization
	explicit BonusDeSpawn(buuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void BonusDeSpawn::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _uuid;
}
}//namespace network::commands
