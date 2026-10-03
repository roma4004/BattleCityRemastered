#pragma once

#include "enums/Faction.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include "enums/TankType.h"
#include <array>
#include <optional>
#include <string_view>

//NOTE: who gets the credit - the seat, not the tank in it; a coop bot scores into the seat it drives
enum class Author : char8_t
{
	//NOTE: no seat - damage that scores for nobody, a grenade above all
	None,

	Enemy1,
	Enemy2,
	Enemy3,
	Enemy4,
	Player1,
	Player2,
	Player3,
	Player4,
	//NOTE: no coop seat - a seat holds one tank at a time, and every counter downstream is per seat

	lastId
};

//NOTE: the wire carries a raw byte, so a value from it lands on no seat unless it names one
[[nodiscard]] constexpr Author SeatFromWire(const Author author) noexcept
{
	return author >= Author::None && author < Author::lastId ? author : Author::None;
}

[[nodiscard]] constexpr Author AuthorOf(const PlayerSlot slot) noexcept
{
	constexpr std::array kAuthors{Author::Player1, Author::Player2, Author::Player3, Author::Player4};

	return kAuthors[SeatIndex(slot)];
}

[[nodiscard]] constexpr std::optional<PlayerSlot> SlotOf(const Author author) noexcept
{
	for (const PlayerSlot slot: kSlots)
	{
		if (author == AuthorOf(slot))
		{
			return slot;
		}
	}

	return std::nullopt;
}

[[nodiscard]] constexpr Faction FactionOf(const Author author) noexcept
{
	switch (author)
	{
		case Author::Enemy1:
		case Author::Enemy2:
		case Author::Enemy3:
		case Author::Enemy4:
			return Faction::EnemyTeam;
		case Author::Player1:
		case Author::Player2:
		case Author::Player3:
		case Author::Player4:
			return Faction::PlayerTeam;
		case Author::None:
		case Author::lastId:
			break;
	}

	return Faction::Neutral;
}

//NOTE: in a free-for-all everyone is Solo
[[nodiscard]] constexpr Faction FactionOf(const Author author, const MatchRules rules) noexcept
{
	return rules == MatchRules::FreeForAll && author != Author::None ? Faction::Solo : FactionOf(author);
}

[[nodiscard]] constexpr Author SeatOf(const TankType type) noexcept
{
	switch (type)
	{
		case TankType::ENEMY1:
			return Author::Enemy1;
		case TankType::ENEMY2:
			return Author::Enemy2;
		case TankType::ENEMY3:
			return Author::Enemy3;
		case TankType::ENEMY4:
			return Author::Enemy4;
		case TankType::PLAYER1:
		case TankType::PLAYER2:
		case TankType::PLAYER3:
		case TankType::PLAYER4:
		case TankType::COOP1:
		case TankType::COOP2:
		case TankType::COOP3:
		case TankType::COOP4:
			return AuthorOf(*SlotOf(type));
	}

	return Author::None;
}

[[nodiscard]] constexpr std::string_view ToString(const Author author) noexcept
{
	switch (author)
	{
		case Author::Enemy1:
			return "Enemy1";
		case Author::Enemy2:
			return "Enemy2";
		case Author::Enemy3:
			return "Enemy3";
		case Author::Enemy4:
			return "Enemy4";
		case Author::Player1:
			return "Player1";
		case Author::Player2:
			return "Player2";
		case Author::Player3:
			return "Player3";
		case Author::Player4:
			return "Player4";
		case Author::None:
		case Author::lastId:
			break;
	}

	return "None";
}
