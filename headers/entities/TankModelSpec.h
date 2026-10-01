#pragma once

#include "enums/TankModel.h"
#include "geometry/Point.h"
#include "utils/MathUtils.h"
#include <algorithm>
#include <chrono>

//NOTE: shares of the base tank, not absolute numbers - TankModel::Player is the base the rest are read against
struct TankModelSpec final
{
	//NOTE: the enemy row within a color quarter - a player's tank takes its row from the tier, not from here
	int spriteRow{};
	double speedFactor{1.0};
	double healthFactor{1.0};
	double bulletSpeedFactor{1.0};
	//NOTE: the heavier the hull, the slower and the harder its shell - the two always move apart
	double damageFactor{1.0};
	//NOTE: of the blast the geometry asks for - a light shell alone no longer opens a passage, the tier grows it
	double blastFactor{1.0};
	//NOTE: across the flight and along it - a scout's round reads as a needle, a heavy's as a log
	double shellCaliberFactor{1.0};
	double shellLengthFactor{1.0};
	//NOTE: of the player's reload, above one is slower - every enemy's is, which keeps a field of four playable
	double reloadFactor{1.0};
};

//NOTE: the original's four types, told apart by hull speed, shell speed, armor and reload
[[nodiscard]] constexpr TankModelSpec SpecOf(const TankModel model) noexcept
{
	switch (model)
	{
		case TankModel::Fast:
			return TankModelSpec{.spriteRow = 1,
								 .speedFactor = 1.6,
								 .healthFactor = 0.2,
								 .bulletSpeedFactor = 1.6,
								 .damageFactor = 1.0 / 3.0,
								 .blastFactor = 0.8,
								 .shellCaliberFactor = 0.6,
								 .shellLengthFactor = 0.8,
								 .reloadFactor = 3.6};
		case TankModel::Power:
			return TankModelSpec{.spriteRow = 2,
								 .healthFactor = 0.5,
								 .blastFactor = 1.3,
								 .shellLengthFactor = 1.2,
								 .reloadFactor = 2.1};
		case TankModel::Armor:
			return TankModelSpec{.spriteRow = 3,
								 .speedFactor = 0.7,
								 .healthFactor = 1.0,
								 .bulletSpeedFactor = 0.75,
								 .damageFactor = 4.0 / 3.0,
								 .blastFactor = 2.0,
								 .shellCaliberFactor = 1.4,
								 .shellLengthFactor = 1.6,
								 .reloadFactor = 3.0};
		//NOTE: the yardstick - every figure of GameConfig is this tank's, so all its shares are one
		case TankModel::Player:
			return TankModelSpec{};
		case TankModel::Basic:
		case TankModel::lastId:
			break;
	}

	//NOTE: also what an unknown model is built as - the wire is the only place one can come from
	return TankModelSpec{.healthFactor = 0.35,
						 .bulletSpeedFactor = 1.2,
						 .damageFactor = 2.0 / 3.0,
						 .reloadFactor = 3.0};
}

//NOTE: a color quarter holds the player's four tier rows, then the four enemy models
inline constexpr int kPlayerTierRows{4};

[[nodiscard]] constexpr int SpriteRowOf(const TankModel model, const unsigned short tier) noexcept
{
	if (model != TankModel::Player)
	{
		return kPlayerTierRows + SpecOf(model).spriteRow;
	}

	return std::clamp(static_cast<int>(tier) - 1, 0, kPlayerTierRows - 1);
}

[[nodiscard]] constexpr double SpeedOf(const TankModel model, const double baseSpeed) noexcept
{
	return baseSpeed * SpecOf(model).speedFactor;
}

//NOTE: rounded, not truncated - a share of the base health has no business landing a point below it
[[nodiscard]] constexpr int HealthOf(const TankModel model, const int baseHealth) noexcept
{
	return MathUtils::RoundTo<int>(baseHealth * SpecOf(model).healthFactor);
}

//NOTE: rounded for the same reason as the health - a third of the base is five points, not four
[[nodiscard]] constexpr unsigned int DamageOf(const TankModel model, const unsigned int baseDamage) noexcept
{
	return MathUtils::RoundTo<unsigned int>(baseDamage * SpecOf(model).damageFactor);
}

[[nodiscard]] constexpr double BlastOf(const TankModel model, const double baseRadius) noexcept
{
	return baseRadius * SpecOf(model).blastFactor;
}

//NOTE: x across the flight and y along it, the way BulletCaliber::size is read everywhere
[[nodiscard]] constexpr FPoint ShellSizeOf(const TankModel model, const FPoint baseSize) noexcept
{
	const auto [across, along]{baseSize};
	const TankModelSpec spec{SpecOf(model)};

	return FPoint{.x = across * spec.shellCaliberFactor, .y = along * spec.shellLengthFactor};
}

[[nodiscard]] constexpr std::chrono::milliseconds ReloadOf(const TankModel model,
														   const std::chrono::milliseconds seatReload) noexcept
{
	return std::chrono::round<std::chrono::milliseconds>(seatReload * SpecOf(model).reloadFactor);
}
