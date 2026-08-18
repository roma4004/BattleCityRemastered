#pragma once

#include "UuidSerialization.h"
#include "enums/CommandType.h"
#include "utils/Uuid.h"
#include <ser20/types/string.hpp>
#include <string>

namespace network::commands
{
class Dispose
{
	CommandType _type{CommandType::DISPOSE};
	std::string _who{};
	Uuid _uuid{};

public:
	//for deserialization
	Dispose() = default;

	//for serialization
	Dispose(std::string who, Uuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] std::string GetWho() const noexcept;
	[[nodiscard]] Uuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & _type;
		ar & _who;
		ar & _uuid;
	}
};
}//namespace network::commands
