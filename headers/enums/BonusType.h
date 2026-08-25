#pragma once

enum class BonusType : char8_t
{
	None,

	Timer,
	Helmet,
	Grenade,
	Tank,
	Star,
	Shovel,
	Caliber,
	Ship,

	lastId
};

//NOTE: the spawnable ids are the interior of the enum - everything strictly between the two ends.
//None is not one of them: it is what a default-constructed BonusType is, that is, "not set yet"
inline constexpr int kFirstSpawnableBonusId{static_cast<int>(BonusType::None) + 1};
inline constexpr int kLastSpawnableBonusId{static_cast<int>(BonusType::lastId) - 1};

//NOTE: the one gate for values that arrive from outside the code - the wire
[[nodiscard]] constexpr bool IsSpawnableBonus(const BonusType type)
{
	const auto id = static_cast<int>(type);

	return id >= kFirstSpawnableBonusId && id <= kLastSpawnableBonusId;
}
