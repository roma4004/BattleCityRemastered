#pragma once

#include <string_view>

enum class Faction : char8_t
{
	Neutral,

	PlayerTeam,
	EnemyTeam,
	//NOTE: a free-for-all tank - everyone's opponent, another Solo too
	Solo,

	lastId
};

[[nodiscard]] constexpr bool IsValidFaction(const Faction faction) noexcept
{
	return faction >= Faction::Neutral && faction < Faction::lastId;
}

[[nodiscard]] constexpr Faction EnemiesOf(const Faction faction) noexcept
{
	switch (faction)
	{
		case Faction::PlayerTeam:
			return Faction::EnemyTeam;
		case Faction::EnemyTeam:
			return Faction::PlayerTeam;
		case Faction::Solo:
			return Faction::Solo;
		case Faction::Neutral:
		case Faction::lastId:
			break;
	}

	return Faction::Neutral;
}

[[nodiscard]] constexpr std::string_view ToString(const Faction faction) noexcept
{
	switch (faction)
	{
		case Faction::PlayerTeam:
			return "PlayerTeam";
		case Faction::EnemyTeam:
			return "EnemyTeam";
		case Faction::Solo:
			return "Solo";
		case Faction::Neutral:
		case Faction::lastId:
			break;
	}

	return "Neutral";
}
