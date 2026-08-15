#pragma once

#include "Point.h"
#include "UuidSerialization.h"
#include "enums/BonusType.h"
#include "enums/CommandType.h"
#include <ser20/types/common.hpp>
#include <boost/uuid/uuid.hpp>

namespace network::commands
{
class BonusSpawn
{
	using buuid = boost::uuids::uuid;

	CommandType _type{CommandType::BONUS_SPAWN};
	FPoint _pos{};
	BonusType _bonusType{};
	buuid _uuid{};

public:
	//for deserialization
	BonusSpawn() = default;

	//for serialization
	BonusSpawn(FPoint pos, BonusType bonusType, buuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] FPoint GetPos() const noexcept;
	[[nodiscard]] BonusType GetBonusType() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void BonusSpawn::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _pos;
	ar & _bonusType;
	ar & _uuid;
}
}//namespace network::commands
