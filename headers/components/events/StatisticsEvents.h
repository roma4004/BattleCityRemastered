#pragma once

#include <string>

enum class Faction : char8_t;

struct BrickWallDiedEvent final
{
	std::string author;
	Faction faction{};
};

struct SteelWallDiedEvent final
{
	std::string author;
	Faction faction{};
};

struct StatisticsBulletHitEvent final
{
	std::string author;
	Faction faction{};
};

struct StatisticsBonusPickupEvent final
{
	std::string author;
	Faction faction{};
};

struct StatisticsBonusDestroyedEvent final
{
	std::string author;
	Faction faction{};
};

//NOTE: bonus just ran out its timeout
struct StatisticsBonusExpiredEvent final {};

struct StatisticsTankHitEvent final
{
	std::string who;
	std::string author;
	Faction faction{};
};

struct StatisticsTankDiedEvent final
{
	std::string who;
	std::string author;
	Faction faction{};
};
