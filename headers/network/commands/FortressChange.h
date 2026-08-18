#pragma once

#include "UuidSerialization.h"
#include "enums/CommandType.h"
#include "enums/FortressState.h"
#include "utils/Uuid.h"

namespace network::commands
{
class FortressChange
{
	CommandType _type{CommandType::FORTRESS_CHANGE};
	FortressState _state{};
	Uuid _uuid{};

public:
	//for deserialization
	FortressChange() = default;

	//for serialization
	FortressChange(FortressState state, Uuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] FortressState GetState() const noexcept;
	[[nodiscard]] Uuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void FortressChange::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _state;
	ar & _uuid;
}
}//namespace network::commands
