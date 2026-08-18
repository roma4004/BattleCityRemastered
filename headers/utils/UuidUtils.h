#pragma once

#include "utils/Uuid.h"
#include <string>
#include <string_view>

//NOTE: the only place allowed to know boost::uuids - everything else speaks Uuid
class UuidUtils final
{
public:
	[[nodiscard]] static Uuid GetRandomUuid();
	[[nodiscard]] static Uuid GetNilUuid();
	[[nodiscard]] static std::string GetStringUuid(Uuid uuid);
	//NOTE: throws on malformed input, so it is for literals in tests and fixtures, not for the wire
	[[nodiscard]] static Uuid GetUuidFromString(std::string_view text);
};
