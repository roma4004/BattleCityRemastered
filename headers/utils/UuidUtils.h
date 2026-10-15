#pragma once

#include "utils/Uuid.h"
#include <string>

//NOTE: the only place allowed to know boost::uuids - everything else speaks Uuid
class UuidUtils final
{
public:
	[[nodiscard]] static Uuid GetRandomUuid();
	[[nodiscard]] static Uuid GetNilUuid() noexcept;
	[[nodiscard]] static std::string GetStringUuid(Uuid uuid);
};
