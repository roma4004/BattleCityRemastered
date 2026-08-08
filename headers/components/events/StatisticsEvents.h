#pragma once

#include <string>

struct ServerSendStatisticsEvent final
{
	std::string eventName;
	std::string author;
	std::string fraction;
};

struct ClientReceivedStatisticsEvent final
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

struct ServerSendBulletHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendEnemyHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendPlayerOneHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendPlayerTwoHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendEnemyDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendPlayerOneDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendPlayerTwoDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendBrickWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendSteelWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendBonusPickupEvent final
{
	std::string author;
	std::string fraction;
};

struct ServerSendBonusDestroyedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedBulletHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedEnemyHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedPlayerOneHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedPlayerTwoHitEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedEnemyDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedPlayerOneDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedPlayerTwoDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedBrickWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedSteelWallDiedEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedBonusPickupEvent final
{
	std::string author;
	std::string fraction;
};

struct ClientReceivedBonusDestroyedEvent final
{
	std::string author;
	std::string fraction;
};
