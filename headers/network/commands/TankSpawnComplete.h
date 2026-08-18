#pragma once

#include "UuidSerialization.h"
#include "enums/CommandType.h"
#include "utils/Uuid.h"

namespace network::commands
{
// Host -> client: "materialize tank uuid now".
class TankSpawnComplete
{
	CommandType _type{CommandType::TANK_SPAWN_COMPLETE};
	Uuid _uuid{};

public:
	//for deserialization
	TankSpawnComplete() = default;

	//for serialization
	explicit TankSpawnComplete(Uuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] Uuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void TankSpawnComplete::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _uuid;
}
}//namespace network::commands
