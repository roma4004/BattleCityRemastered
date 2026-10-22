#pragma once

#include "components/StatisticsData.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "enums/Absence.h"
#include "enums/Author.h"
#include "enums/Delivery.h"
#include "enums/Direction.h"
#include "enums/GameState.h"
#include "enums/ObstacleType.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "enums/TankModel.h"
#include "enums/TankType.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include "utils/Uuid.h"
#include <array>
#include <vector>

namespace network::commands
{
struct TankSnapshot final
{
	TankType type{};
	TankModel model{};
	Uuid uuid{};
	ObjRectangle rect{};
	Direction dir{};
	unsigned int health{};
	unsigned short tier{};
	bool isHelmetActive{};
	bool isShipActive{};
};

struct ObstacleSnapshot final
{
	FPoint pos{};
	ObstacleType type{};
	Uuid uuid{};
	unsigned int health{};
};

struct BulletSnapshot final
{
	Author author{};
	Uuid uuid{};
	ObjRectangle rect{};
	Direction dir{};
};

//NOTE: the host's field for a client to rebuild from scratch; serialized in CommandSerialization.h
struct WorldSnapshot final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	//NOTE: applied first - a client never reads a map, and everything else here lands on the field this sizes
	MapLoadedEvent map{};
	GameState phase{};
	std::vector<ObstacleSnapshot> obstacles{};
	std::vector<TankSnapshot> tanks{};
	std::vector<TankRespawnedEvent> tankSpawns{};
	std::vector<BulletSnapshot> bullets{};
	std::vector<BonusSpawnedEvent> bonuses{};
	std::vector<BonusSpawnedEvent> bonusSpawns{};
	std::vector<BonusEffectTimeLeft> effects{};
	//NOTE: indexed by RespawnGroup
	std::array<unsigned short, kRespawnGroupCount> respawnCounts{};
	StatisticsData statistics{};
	//NOTE: the seat announce may be in the frame this snapshot replaces, so the snapshot carries it too
	std::array<SeatHolder, kSeatCount> holders{};
	//NOTE: a newcomer to a held match is shown why it is held
	std::array<Absence, kSeatCount> absence{};
};
}//namespace network::commands
