#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <typeindex>//NOTE: cheapest standard header that declares std::hash

//NOTE: vocabulary uuid for headers. Layout is identical to boost::uuids::uuid - 16 bytes, 8-byte
//aligned - so the two convert by memcpy/bit_cast, and boost stays behind UuidUtils.cpp. The point is
//parse cost: <boost/uuid/uuid.hpp> is ~92k lines after preprocessing and was stored by value in 28
//of our headers.
struct alignas(std::uint64_t) Uuid final
{
	std::uint8_t data[16]{};

	[[nodiscard]] friend bool operator==(const Uuid& lhs, const Uuid& rhs) = default;
	[[nodiscard]] friend std::strong_ordering operator<=>(const Uuid& lhs, const Uuid& rhs) = default;
};

static_assert(sizeof(Uuid) == 16, "Uuid must stay wire- and boost-compatible");
static_assert(alignof(Uuid) == 8, "Uuid must keep boost::uuids::uuid's alignment");

template<>
struct std::hash<Uuid>
{
	//NOTE: local map keys only, never serialized - the mix may change without breaking the wire
	[[nodiscard]] std::size_t operator()(const Uuid& uuid) const noexcept
	{
		std::uint64_t low{};
		std::uint64_t high{};
		std::memcpy(&low, uuid.data, sizeof(low));
		std::memcpy(&high, uuid.data + sizeof(low), sizeof(high));

		std::uint64_t mixed = low ^ (high + 0x9e3779b97f4a7c15ULL + (low << 6u) + (low >> 2u));
		mixed ^= mixed >> 33u;
		mixed *= 0xff51afd7ed558ccdULL;
		mixed ^= mixed >> 33u;

		return static_cast<std::size_t>(mixed);
	}
};
