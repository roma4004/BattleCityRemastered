#pragma once

#include <string_view>

//NOTE: what the tank is, not who drives it - the seat is TankType, and the two are chosen apart
enum class TankModel : char8_t
{
	Basic,
	Fast,
	Power,
	Armor,
	//NOTE: the person's tank and the yardstick - past the rolled ones, so no enemy can roll the player's numbers
	Player,

	lastId
};

inline constexpr int kFirstTankModelId{static_cast<int>(TankModel::Basic)};
inline constexpr int kLastEnemyModelId{static_cast<int>(TankModel::Armor)};

[[nodiscard]] constexpr std::string_view ToString(const TankModel model) noexcept
{
	switch (model)
	{
		case TankModel::Fast:
			return "Fast";
		case TankModel::Power:
			return "Power";
		case TankModel::Armor:
			return "Armor";
		case TankModel::Player:
			return "Player";
		case TankModel::Basic:
		case TankModel::lastId:
			break;
	}

	return "Basic";
}
