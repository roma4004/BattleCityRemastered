#pragma once

#include <string>

struct BrickWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct SteelWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct StatisticsBulletHitEvent final
{
	std::string author;
	std::string fraction;
};

struct StatisticsBonusPickupEvent final
{
	std::string author;
	std::string fraction;
};

struct StatisticsBonusDestroyedEvent final
{
	std::string author;
	std::string fraction;
};

//NOTE: bonus just ran out its timeout
struct StatisticsBonusExpiredEvent final {};

struct StatisticsTankHitEvent final
{
	std::string who;
	std::string author;
	std::string fraction;
};

struct StatisticsTankDiedEvent final
{
	std::string who;
	std::string author;
	std::string fraction;
};
