#pragma once

#include "BonusSpawn.h"
#include "BonusSpawnComplete.h"
#include "BonusStatus.h"
#include "Despawn.h"
#include "Disconnect.h"
#include "GameStateChange.h"
#include "HealthChange.h"
#include "KeyStateChange.h"
#include "ObstacleSpawn.h"
#include "PositionChange.h"
#include "RespawnTank.h"
#include "SignalEvent.h"
#include "SlotAssignment.h"
#include "StatisticsChange.h"
#include "TankShot.h"
#include "TankSpawnComplete.h"
#include "TankSpawnMoved.h"
#include "TierChange.h"
#include "PointSerialization.h"
#include "CommandBatch.h"
#include "UuidSerialization.h"
#include "components/StatisticsData.h"
#include "components/WorldSnapshot.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "geometry/ObjRectangle.h"
#include <ser20/types/array.hpp>
#include <ser20/types/common.hpp>
#include <ser20/types/string.hpp>
#include <ser20/types/variant.hpp>
#include <ser20/types/vector.hpp>

namespace ser20
{
//NOTE: every command in one place, so a command header stays plain data and ser20 reaches only
//this file and the Serializer.

template<class Archive>
void serialize(Archive& ar, network::commands::BonusSpawn& cmd, const unsigned int /*version*/)
{
	ar & cmd.pos;
	ar & cmd.bonusType;
	ar & cmd.uuid;
	ar & cmd.isSuper;
}

template<class Archive>
void serialize(Archive& ar, network::commands::BonusStatus& cmd, const unsigned int /*version*/)
{
	ar & cmd.author;
	ar & cmd.bonusType;
	ar & cmd.isEnable;
}

template<class Archive>
void serialize(Archive& ar, network::commands::Disconnect& cmd, const unsigned int /*version*/)
{
	ar & cmd.reason;
}

template<class Archive>
void serialize(Archive& ar, network::commands::Despawn& cmd, const unsigned int /*version*/)
{
	ar & cmd.uuid;
	ar & cmd.reason;
}

template<class Archive>
void serialize(Archive& ar, network::commands::GameStateChange& cmd, const unsigned int /*version*/)
{
	ar & cmd.state;
}

template<class Archive>
void serialize(Archive& ar, network::commands::HealthChange& cmd, const unsigned int /*version*/)
{
	ar & cmd.health;
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::KeyStateChange& cmd, const unsigned int /*version*/)
{
	ar & cmd.action;
	ar & cmd.isPressed;
}

template<class Archive>
void serialize(Archive& ar, network::commands::ObstacleSpawn& cmd, const unsigned int /*version*/)
{
	ar & cmd.pos;
	ar & cmd.obstacleType;
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::PositionChange& cmd, const unsigned int /*version*/)
{
	ar & cmd.pos;
	ar & cmd.dir;
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::RespawnTank& cmd, const unsigned int /*version*/)
{
	ar & cmd.tankType;
	ar & cmd.model;
	ar & cmd.uuid;
	ar & cmd.pos;
}

template<class Archive>
void serialize(Archive& ar, network::commands::SignalEvent& cmd, const unsigned int /*version*/)
{
	ar & cmd.signal;
}

template<class Archive>
void serialize(Archive& ar, MatchSettings& match, const unsigned int /*version*/)
{
	ar & match.rules;
	ar & match.seats;
	ar & match.map;
	ar & match.enemiesAtOnce;
}

template<class Archive>
void serialize(Archive& ar, network::commands::SlotAssignment& cmd, const unsigned int /*version*/)
{
	ar & cmd.slot;
	ar & cmd.match;
}

template<class Archive>
void serialize(Archive& ar, network::commands::StatisticsChange& cmd, const unsigned int /*version*/)
{
	ar & cmd.statisticsType;
	ar & cmd.who;
	ar & cmd.author;
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::TankShot& cmd, const unsigned int /*version*/)
{
	ar & cmd.who;
	ar & cmd.dir;
	ar & cmd.uuid;
	ar & cmd.damage;
}

template<class Archive>
void serialize(Archive& ar, network::commands::TankSpawnComplete& cmd, const unsigned int /*version*/)
{
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::TankSpawnMoved& cmd, const unsigned int /*version*/)
{
	ar & cmd.uuid;
	ar & cmd.pos;
}

template<class Archive>
void serialize(Archive& ar, network::commands::BonusSpawnComplete& cmd, const unsigned int /*version*/)
{
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::TierChange& cmd, const unsigned int /*version*/)
{
	ar & cmd.tier;
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, ObjRectangle& rect, const unsigned int /*version*/)
{
	ar & rect.x;
	ar & rect.y;
	ar & rect.w;
	ar & rect.h;
}

template<class Archive>
void serialize(Archive& ar, ObstacleSnapshot& obstacle, const unsigned int /*version*/)
{
	ar & obstacle.pos;
	ar & obstacle.type;
	ar & obstacle.uuid;
	ar & obstacle.health;
}

template<class Archive>
void serialize(Archive& ar, TankRespawnedEvent& spawn, const unsigned int /*version*/)
{
	ar & spawn.type;
	ar & spawn.model;
	ar & spawn.uuid;
	ar & spawn.pos;
}

template<class Archive>
void serialize(Archive& ar, BonusSpawnedEvent& bonus, const unsigned int /*version*/)
{
	ar & bonus.pos;
	ar & bonus.type;
	ar & bonus.uuid;
	ar & bonus.isSuper;
}

template<class Archive>
void serialize(Archive& ar, TankSnapshot& tank, const unsigned int /*version*/)
{
	ar & tank.type;
	ar & tank.model;
	ar & tank.uuid;
	ar & tank.pos;
	ar & tank.dir;
	ar & tank.health;
	ar & tank.tier;
	ar & tank.isHelmetActive;
	ar & tank.isShipActive;
}

template<class Archive>
void serialize(Archive& ar, BulletSnapshot& bullet, const unsigned int /*version*/)
{
	ar & bullet.author;
	ar & bullet.uuid;
	ar & bullet.rect;
	ar & bullet.dir;
}

template<class Archive>
void serialize(Archive& ar, SeatStatistics& seat, const unsigned int /*version*/)
{
	ar & seat.bulletHits;
	ar & seat.enemyHits;
	ar & seat.enemyKills;
	ar & seat.hitByEnemyTeam;
	ar & seat.friendlyHitsTaken;
	ar & seat.friendlyKillsTaken;
	ar & seat.brickWallKills;
	ar & seat.steelWallKills;
	ar & seat.bonusPickups;
	ar & seat.bonusesDestroyed;
}

template<class Archive>
void serialize(Archive& ar, EnemyTeamStatistics& team, const unsigned int /*version*/)
{
	ar & team.bulletHits;
	ar & team.playerKills;
	ar & team.friendlyHitsTaken;
	ar & team.friendlyKillsTaken;
	ar & team.brickWallKills;
	ar & team.steelWallKills;
	ar & team.bonusPickups;
	ar & team.bonusesDestroyed;
}

template<class Archive>
void serialize(Archive& ar, StatisticsData& data, const unsigned int /*version*/)
{
	ar & data.seats;
	ar & data.enemyTeam;
	ar & data.bonusExpired;
}

template<class Archive>
void serialize(Archive& ar, MapLoadedEvent& map, const unsigned int /*version*/)
{
	ar & map.cols;
	ar & map.rows;
	ar & map.stage;
}

template<class Archive>
void serialize(Archive& ar, WorldSnapshot& snapshot, const unsigned int /*version*/)
{
	ar & snapshot.map;
	ar & snapshot.phase;
	ar & snapshot.obstacles;
	ar & snapshot.tanks;
	ar & snapshot.tankSpawns;
	ar & snapshot.bullets;
	ar & snapshot.bonuses;
	ar & snapshot.bonusSpawns;
	ar & snapshot.respawnCounts;
	ar & snapshot.statistics;
}

template<class Archive>
void serialize(Archive& ar, network::commands::CommandBatch& batch, const unsigned int /*version*/)
{
	ar & batch.commands;
}
}// namespace ser20
