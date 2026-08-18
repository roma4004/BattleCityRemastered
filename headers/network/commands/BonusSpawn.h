#pragma once

#include "Point.h"
#include "UuidSerialization.h"
#include "enums/BonusType.h"
#include "enums/CommandType.h"
#include "utils/Uuid.h"
#include <ser20/types/common.hpp>

namespace network::commands
{
class BonusSpawn
{
	CommandType _type{CommandType::BONUS_SPAWN};
	FPoint _pos{};
	BonusType _bonusType{};
	Uuid _uuid{};

public:
	//for deserialization
	BonusSpawn() = default;

	//for serialization
	BonusSpawn(FPoint pos, BonusType bonusType, Uuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] FPoint GetPos() const noexcept;
	[[nodiscard]] BonusType GetBonusType() const noexcept;
	[[nodiscard]] Uuid GetUuid() const noexcept;
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
