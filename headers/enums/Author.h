#pragma once

#include "enums/Faction.h"
#include "enums/TankType.h"
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
	//NOTE: no coop seat - a seat holds one tank at a time, and every counter downstream is per seat

	lastId
};

//NOTE: the wire carries a raw byte, so a value from it lands on no seat unless it names one
[[nodiscard]] constexpr Author SeatFromWire(const Author author)
{
	return author >= Author::None && author < Author::lastId ? author : Author::None;
}

//NOTE: the team is a property of the seat, never a second field next to it
[[nodiscard]] constexpr Faction FactionOf(const Author author)
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
			return Faction::PlayerTeam;
		case Author::None:
		case Author::lastId:
			break;
	}

	return Faction::Neutral;
}

//NOTE: many to one - both tank types that can drive a player's seat answer to the same author
[[nodiscard]] constexpr Author SeatOf(const TankType type)
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
		case TankType::COOP1:
			return Author::Player1;
		case TankType::PLAYER2:
		case TankType::COOP2:
			return Author::Player2;
	}

	return Author::None;
}

[[nodiscard]] constexpr std::string_view ToString(const Author author)
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
		case Author::None:
		case Author::lastId:
			break;
	}

	return "None";
}
