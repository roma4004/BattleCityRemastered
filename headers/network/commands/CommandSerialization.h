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
#include "StatisticsChange.h"
#include "TankShot.h"
#include "TankSpawnComplete.h"
#include "TierChange.h"
#include "PointSerialization.h"
#include "CommandBatch.h"
#include "UuidSerialization.h"
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
	ar & cmd.name;
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
	ar & cmd.who;
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
	ar & cmd.who;
	ar & cmd.health;
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::KeyStateChange& cmd, const unsigned int /*version*/)
{
	ar & cmd.tag;
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
	ar & cmd.who;
	ar & cmd.pos;
	ar & cmd.dir;
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::RespawnTank& cmd, const unsigned int /*version*/)
{
	ar & cmd.tankType;
	ar & cmd.uuid;
	ar & cmd.pos;
}

template<class Archive>
void serialize(Archive& ar, network::commands::SignalEvent& cmd, const unsigned int /*version*/)
{
	ar & cmd.signal;
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
}

template<class Archive>
void serialize(Archive& ar, network::commands::TankSpawnComplete& cmd, const unsigned int /*version*/)
{
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::BonusSpawnComplete& cmd, const unsigned int /*version*/)
{
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::TierChange& cmd, const unsigned int /*version*/)
{
	ar & cmd.who;
	ar & cmd.tier;
	ar & cmd.uuid;
}

template<class Archive>
void serialize(Archive& ar, network::commands::CommandBatch& batch, const unsigned int /*version*/)
{
	ar & batch.commands;
}
}// namespace ser20
