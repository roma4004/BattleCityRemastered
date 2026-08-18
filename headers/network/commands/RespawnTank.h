#pragma once

#include "UuidSerialization.h"
#include "entities/ObjRectangle.h"
#include "enums/CommandType.h"
#include "utils/Uuid.h"
#include <ser20/types/common.hpp>

enum class TankType : char8_t;

namespace network::commands
{
class RespawnTank
{
	CommandType _type{CommandType::RESPAWN_TANK};
	TankType _tankType{};
	Uuid _uuid{};
	ObjRectangle _rect{};

public:
	//for deserialization
	RespawnTank() = default;

	//for serialization
	RespawnTank(TankType tankType, Uuid uuid, ObjRectangle rect);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] TankType GetTankType() const noexcept;
	[[nodiscard]] Uuid GetUuid() const noexcept;
	[[nodiscard]] ObjRectangle GetRect() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void RespawnTank::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _tankType;
	ar & _uuid;
	ar & _rect;
}
}//namespace network::commands
