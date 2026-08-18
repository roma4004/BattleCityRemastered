#include "utils/UuidUtils.h"

#include <bit>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

using buuid = boost::uuids::uuid;

static_assert(sizeof(buuid) == sizeof(Uuid), "Uuid and boost::uuids::uuid must stay layout-compatible");

namespace
{
[[nodiscard]] Uuid ToUuid(const buuid uuid) { return std::bit_cast<Uuid>(uuid); }

[[nodiscard]] buuid ToBoost(const Uuid uuid) { return std::bit_cast<buuid>(uuid); }
}// namespace

Uuid UuidUtils::GetRandomUuid()
{
	static boost::uuids::random_generator uuidGenerator;

	return ToUuid(uuidGenerator());
}

//NOTE: boost's nil is all-zero bytes, which is exactly a default-constructed Uuid - no boost needed
Uuid UuidUtils::GetNilUuid() { return Uuid{}; }

std::string UuidUtils::GetStringUuid(const Uuid uuid) { return boost::uuids::to_string(ToBoost(uuid)); }

Uuid UuidUtils::GetUuidFromString(const std::string_view text)
{
	return ToUuid(boost::uuids::string_generator{}(text.begin(), text.end()));
}
