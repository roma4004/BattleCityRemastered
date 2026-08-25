#pragma once

#include <string_view>

enum class Faction : char8_t
{
	Neutral,

	PlayerTeam,
	EnemyTeam,

	lastId
};

[[nodiscard]] constexpr bool IsValidFaction(const Faction faction)
{
	return faction >= Faction::Neutral && faction < Faction::lastId;
}

[[nodiscard]] constexpr Faction EnemiesOf(const Faction faction)
{
	switch (faction)
	{
		case Faction::PlayerTeam:
			return Faction::EnemyTeam;
		case Faction::EnemyTeam:
			return Faction::PlayerTeam;
		case Faction::Neutral:
		case Faction::lastId:
			break;
	}

	return Faction::Neutral;
}

[[nodiscard]] constexpr std::string_view ToString(const Faction faction)
{
	switch (faction)
	{
		case Faction::PlayerTeam:
			return "PlayerTeam";
		case Faction::EnemyTeam:
			return "EnemyTeam";
		case Faction::Neutral:
		case Faction::lastId:
			break;
	}

	return "Neutral";
}
