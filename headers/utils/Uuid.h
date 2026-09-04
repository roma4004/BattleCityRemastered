#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <typeindex>//NOTE: required by GCC only - cheapest header declaring std::hash; MSVC-STL leaks it

//NOTE: our own 16 bytes instead of boost::uuids::uuid - that header is ~92k lines preprocessed
struct Uuid final
{
	std::uint8_t data[16]{};

	[[nodiscard]] friend bool operator==(const Uuid& lhs, const Uuid& rhs) = default;
	[[nodiscard]] friend std::strong_ordering operator<=>(const Uuid& lhs, const Uuid& rhs) = default;
};

static_assert(sizeof(Uuid) == 16, "Uuid must stay wire- and boost-compatible");

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
