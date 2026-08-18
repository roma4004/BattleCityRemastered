#pragma once

#include "Point.h"
#include "UuidSerialization.h"
#include "enums/CommandType.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"
#include <ser20/types/common.hpp>
#include <ser20/types/string.hpp>
#include <string>

namespace network::commands
{
class PositionChange
{
	CommandType _type{CommandType::POSITION_CHANGE};
	std::string _who{};
	FPoint _pos{};
	Direction _dir{};
	Uuid _uuid{};

public:
	//for deserialization
	PositionChange() = default;

	//for serialization
	PositionChange(std::string who, FPoint pos, Direction dir, Uuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] std::string GetWho() const noexcept;
	[[nodiscard]] FPoint GetPos() const noexcept;
	[[nodiscard]] Direction GetDir() const noexcept;
	[[nodiscard]] Uuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void PositionChange::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _who;
	ar & _pos;
	ar & _dir;
	ar & _uuid;
}
}//namespace network::commands
