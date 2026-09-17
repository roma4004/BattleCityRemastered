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

//NOTE: spawnable ids lie strictly between None, which means "not set yet", and lastId
inline constexpr int kFirstSpawnableBonusId{static_cast<int>(BonusType::None) + 1};
inline constexpr int kLastSpawnableBonusId{static_cast<int>(BonusType::lastId) - 1};

//NOTE: the one gate for values that arrive from outside the code - the wire
[[nodiscard]] constexpr bool IsSpawnableBonus(const BonusType type)
{
	const auto id{static_cast<int>(type)};

	return id >= kFirstSpawnableBonusId && id <= kLastSpawnableBonusId;
}
