#pragma once

#include <string>

struct ServerOutStatisticsEvent final
{
	std::string eventName;
	std::string author;
	std::string fraction;
};

struct ClientInStatisticsEvent final
{
	std::string eventName;
	std::string author;
	std::string fraction;
};

struct TankStatisticsEvent final
{
	std::string who;
	std::string author;
	std::string fraction;
};

//NOTE: Obstacle.cpp used to build this event's name dynamically as "Statistics_" + _name + "Died";
//_name is closed to {"BrickWall", "SteelWall"}, so two fixed structs replace it.
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

struct ServerOutBulletHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutEnemyHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutPlayerOneHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutPlayerTwoHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutEnemyDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutPlayerOneDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutPlayerTwoDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutBrickWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutSteelWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutBonusPickupEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerOutBonusDestroyedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInBulletHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInEnemyHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInPlayerOneHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInPlayerTwoHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInEnemyDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInPlayerOneDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInPlayerTwoDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInBrickWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInSteelWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInBonusPickupEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientInBonusDestroyedEvent final
{
	std::string author;
	std::string fraction;
};
