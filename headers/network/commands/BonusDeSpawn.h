#pragma once

#include "UuidSerialization.h"
#include "enums/CommandType.h"
#include "utils/Uuid.h"

namespace network::commands
{
class BonusDeSpawn
{
	CommandType _type{CommandType::BONUS_DESPAWN};
	Uuid _uuid{};

public:
	//for deserialization
	BonusDeSpawn() = default;

	//for serialization
	explicit BonusDeSpawn(Uuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] Uuid GetUuid() const noexcept;
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
