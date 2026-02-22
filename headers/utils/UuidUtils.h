#pragma once

#include <string>

namespace boost::uuids
{
struct uuid;
}

class UuidUtils final
{
	using buuid = boost::uuids::uuid;

public:
	[[nodiscard]] static buuid GetRandomUuid();
	[[nodiscard]] static buuid GetNilUuid();
	[[nodiscard]] static std::string GetStringUuid(buuid uuid);
};
