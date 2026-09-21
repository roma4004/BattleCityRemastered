#include "components/TankSpawner.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/TankPool.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/input/InputProviderForBot.h"
#include "components/input/InputProviderForPlayer.h"
#include "components/events/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/WorldGeometry.h"
#include "components/WorldSnapshot.h"
#include "entities/pawns/Tank.h"
#include "entities/pawns/TankResetProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/InputChannel.h"
#include "enums/PlayerSlot.h"
#include "enums/TankType.h"
#include "enums/Faction.h"
#include "utils/Log.h"
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "utils/WorldQuery.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace
{
//NOTE: every cell of the strip plus the rolled point itself, the nearest to it first - a spot beside the
//roll still reads as that spawn, a spot across the field does not
[[nodiscard]] std::vector<double> SpawnCandidates(const double minX, const double maxX, const double preferredX,
												  const double cell)
{
	const double first{std::ceil(minX / cell) * cell};
	const auto steps{static_cast<int>((maxX - first) / cell)};

	auto candidates{std::views::iota(0, steps + 1)
					| std::views::transform([first, cell](const int step) { return first + step * cell; })
					| std::ranges::to<std::vector>()};
	candidates.push_back(preferredX);
	std::ranges::sort(candidates, {}, [preferredX](const double x) { return std::abs(x - preferredX); });

	return candidates;
}
}// namespace

//NOTE: the pools come from outside - this class is rebuilt on every mode change and they are not
TankSpawner::TankSpawner(const GameConfig& gameConfig, const std::vector<std::shared_ptr<BaseObj>>& allObjects,
						 const std::shared_ptr<EventSystem>& events, const std::shared_ptr<TankPool>& tankPool)
	: _allObjects{allObjects}
	, _events{events}
	, _tankPool{tankPool}
	, _gameMode{gameConfig.gameMode}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

void TankSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &TankSpawner::Reset));
	_subs.push_back(_events->AddListener(this, &TankSpawner::OnRespawnTank));
	//NOTE: a tank is the host's call - the client's burst never finishes and waits for TankSpawnComplete
	if (IsAuthority(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnSpawnAnimationFinished));
	}

	if (IsClient(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankRespawned));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankSpawnCompleted));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankDied));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnWorldSnapshotReceived));
	}

	if (IsHost(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnWorldSnapshotRequested));
	}

	//NOTE: a tank mid-spawn is not an object yet, so the grenade cannot reach it the way it reaches
	//the others - the spawner cancels it on their behalf, and only where the world is decided
	if (IsAuthority(_gameMode))
	{
		for (const Faction faction: {Faction::PlayerTeam, Faction::EnemyTeam})
		{
			_subs.push_back(_events->AddListener(Key(faction), [this, faction](const BonusGrenadePickupEvent&)
			{
				CancelDelayedSpawnsOf(faction);
			}));
		}
	}
}

void TankSpawner::OnRespawnTank(const RespawnTankEvent& event) { RespawnTank(event.type, event.uuid); }

void TankSpawner::OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event)
{
	OnSpawnDelayFinished(event.uuid);
}

void TankSpawner::OnTankRespawned(const TankRespawnedEvent& event)
{
	const double tankSize{_gameConfig.tankSize};
	OnClientRespawn(event.type, event.uuid,
					ObjRectangle{.x = event.pos.x, .y = event.pos.y, .w = tankSize, .h = tankSize});
}

void TankSpawner::OnTankSpawnCompleted(const TankSpawnCompletedEvent& event)
{
	OnSpawnDelayFinished(event.uuid);
}

void TankSpawner::OnTankDied(const TankDiedEvent& event) { DropDelayedSpawn(event.uuid); }

void TankSpawner::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	std::ranges::transform(_delayedSpawns, std::back_inserter(event.snapshot.tankSpawns),
						   [](const DelayedTankSpawn& spawn)
						   {
							   return TankRespawnedEvent{.type = spawn.type,
														 .uuid = spawn.uuid,
														 .pos = FPoint{.x = spawn.rect.x, .y = spawn.rect.y}};
						   });
}

//NOTE: a spawn still bursting goes the way a mirrored respawn does, minus the event - the snapshot's lives
//already count it
void TankSpawner::OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event)
{
	std::ranges::for_each(event.snapshot.tanks, [this](const TankSnapshot& tank) { RestoreTank(tank); });
	std::ranges::for_each(event.snapshot.tankSpawns, [this](const TankRespawnedEvent& spawn)
	{
		OnTankRespawned(spawn);
	});
}

void TankSpawner::Reset(const GameResetEvent&)
{
	_enemySpawnTimer.cooldown = _gameConfig.enemySpawnCooldown;
	_enemySpawnTimer.isActive = false;
	_enemySpawnTimer.activateTime = TimeUtils::Now() - _enemySpawnTimer.cooldown;
	_delayedSpawns.clear();
}

std::optional<ObjRectangle> TankSpawner::FindSpawnSpot(const double minX, const double maxX, const double y,
													   const double preferredX) const
{
	const double tankSize{_gameConfig.tankSize};
	auto isFree = [this, y, tankSize](const double x)
	{
		return WorldQuery::IsSpotFreeOfBlockers(_allObjects,
											   ObjRectangle{.x = x, .y = y, .w = tankSize, .h = tankSize});
	};

	const std::vector<double> candidates{SpawnCandidates(minX, maxX, preferredX, _gameConfig.gridOffset)};
	const auto found{std::ranges::find_if(candidates, isFree)};
	if (found == candidates.end())
	{
		return std::nullopt;
	}

	return ObjRectangle{.x = *found, .y = y, .w = tankSize, .h = tankSize};
}

std::optional<ObjRectangle> TankSpawner::GetEnemyRandomPosX(const TankType type) const
{
	if (type > TankType::ENEMY4)
	{
		return std::nullopt;
	}

	const double tankSize{_gameConfig.tankSize};
	const double battleFieldSizeX{static_cast<double>(_gameConfig.battlefieldSize.x) - tankSize};
	const double quartFieldSizeX{battleFieldSizeX / 4.0};

	//NOTE: a quarter per seat, so the four of them come in spread across the front
	const auto quarter{static_cast<double>(type)};
	const double minX{quarter * quartFieldSizeX};
	const double maxX{minX + quartFieldSizeX};

	const std::uniform_real_distribution<double> distRandX{minX, maxX};
	const double randomX{RandUtils::GetRandNumber(distRandX)};

	//NOTE: the whole front only once its own quarter is full - no tank at all is worse than one out of place
	return FindSpawnSpot(minX, maxX, 0.0, randomX)
		   .or_else([this, battleFieldSizeX, randomX] { return FindSpawnSpot(0.0, battleFieldSizeX, 0.0, randomX); });
}

bool TankSpawner::SpawnEnemy(const ObjRectangle rect, const Uuid uuid, const TankType type, const double speed,
							 const int health)
{
	const std::string name{"Enemy" + std::to_string(static_cast<int>(type) + 1)};
	constexpr auto faction{Faction::EnemyTeam};

	Log::Info("spawn " + name + " (" + std::string{ToString(faction)} + ") uuid " + UuidUtils::GetStringUuid(uuid));

	DelayedSpawnStart(rect, health, speed, uuid, type);

	return true;
}

void TankSpawner::SpawnPlayer(const ObjRectangle rect, const double speed, const int health, const Uuid uuid,
							  const TankType type)
{
	const bool isFirst{type == TankType::PLAYER1};
	const std::string name{isFirst ? "Player1" : "Player2"};
	constexpr auto faction{Faction::PlayerTeam};

	Log::Info("spawn " + name + " (" + std::string{ToString(faction)} + ") uuid " + UuidUtils::GetStringUuid(uuid));

	DelayedSpawnStart(rect, health, speed, uuid, type);
}

void TankSpawner::SpawnCoopBot(const ObjRectangle rect, const double speed, const int health, const Uuid uuid,
							   const TankType type)
{
	const std::string name{(type == TankType::COOP1 ? "CoopBot1" : "CoopBot2")};
	constexpr auto faction{Faction::PlayerTeam};

	Log::Info("spawn " + name + " (" + std::string{ToString(faction)} + ") uuid " + UuidUtils::GetStringUuid(uuid));

	DelayedSpawnStart(rect, health, speed, uuid, type);
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const Uuid uuid,
									const std::optional<ObjRectangle> rect)
{
	const std::optional<ObjRectangle> spawnRect{rect.has_value() ? rect : GetEnemyRandomPosX(type)};
	if (!spawnRect)
	{
		return;
	}

	const bool isSuccessSpawn{SpawnEnemy(*spawnRect, uuid, type, _gameConfig.tankSpeed, _gameConfig.tankHealth)};
	if (isSuccessSpawn && IsHost(_gameMode))
	{
		_events->EmitEvent(TankRespawnedEvent{.type = type,
											  .uuid = uuid,
											  .pos = FPoint{.x = spawnRect->x, .y = spawnRect->y}});
	}
}

//TODO: write unit test for bot change direction if faced obstacle
std::optional<ObjRectangle> TankSpawner::GetPlayerRandomPosX(const bool isFirst) const
{
	const auto battleFieldSizeX{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto battleFieldSizeY{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const double tankSize{_gameConfig.tankSize};

	//NOTE: the seats stand either side of the fortress, so the ranges are cut around it - the eagle plus its
	//ring of walls, from the middle of the field outwards
	const double fortressHalfWidth{_gameConfig.gridOffset
								   * (ObstacleCellSpan(ObstacleType::Eagle) / 2.0
									  + static_cast<double>(WorldGeometry::kFortressRingCells))};
	const double middle{battleFieldSizeX / 2.0};

	const std::pair spawnRangePlayer1{0.0, middle - fortressHalfWidth - tankSize};
	const std::pair spawnRangePlayer2{middle + fortressHalfWidth, battleFieldSizeX - tankSize};
	const auto [minX, maxX]{isFirst ? spawnRangePlayer1 : spawnRangePlayer2};

	const std::uniform_real_distribution distRandId{minX, maxX};
	const double randomX{RandUtils::GetRandNumber(distRandId)};

	return FindSpawnSpot(minX, maxX, battleFieldSizeY - tankSize, randomX);
}

void TankSpawner::RespawnPlayerTeam(const TankType type, const Uuid uuid,
									const std::optional<ObjRectangle> rect)
{
	const bool isFirst{type == TankType::PLAYER1};
	const std::optional<ObjRectangle> spawnRect{rect.has_value() ? rect : GetPlayerRandomPosX(isFirst)};
	if (!spawnRect)
	{
		return;
	}

	//NOTE: the demo is the phase where nobody sits down - every seat goes to a bot
	const bool isDemo{_gameConfig.gameState == GameState::Demo};
	if (!isDemo
		&& (_gameMode == GameMode::OnePlayer
			|| _gameMode == GameMode::TwoPlayers
			|| IsNetworkGame(_gameMode)
			|| (_gameMode == GameMode::CoopWithBot && isFirst)))
	{
		SpawnPlayer(*spawnRect, _gameConfig.tankSpeed, _gameConfig.tankHealth, uuid, type);
		if (IsHost(_gameMode))
		{
			_events->EmitEvent(TankRespawnedEvent{.type = type,
												  .uuid = uuid,
												  .pos = FPoint{.x = spawnRect->x, .y = spawnRect->y}});
		}
	}
	else if (UsesCoopBots(_gameMode))
	{
		SpawnCoopBot(*spawnRect, _gameConfig.tankSpeed, _gameConfig.tankHealth, uuid,
					 isFirst ? TankType::COOP1 : TankType::COOP2);
	}
}

void TankSpawner::RespawnTank(const TankType type, const Uuid uuid, const std::optional<ObjRectangle> rect)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
		{
			if (const bool isNetworkMirrored{rect.has_value()};// server already decided when to spawn this tank
				isNetworkMirrored)
			{
				RespawnEnemyTanks(type, uuid, rect);
			}
			else if (_enemySpawnTimer.IsCooldownFinish())
			{
				_enemySpawnTimer.Reset();
				RespawnEnemyTanks(type, uuid, rect);
			}
			break;
		}
		case TankType::PLAYER1:
		case TankType::PLAYER2:
		case TankType::COOP1:
		case TankType::COOP2:
			RespawnPlayerTeam(type, uuid, rect);
			break;
	}
}

//TODO: maybe we don't need spawn on client at all and just move the textures and animation?
void TankSpawner::OnClientRespawn(const TankType type, const Uuid uuid, const ObjRectangle rect)
{
	RespawnTank(type, uuid, rect);
}

std::unique_ptr<IInputProvider> TankSpawner::MakeDriver(const TankType type) const
{
	if (type != TankType::PLAYER1 && type != TankType::PLAYER2)
	{
		return std::make_unique<InputProviderForBot>(_allObjects, _gameConfig);
	}

	const PlayerSlot slot{type == TankType::PLAYER1 ? PlayerSlot::P1 : PlayerSlot::P2};

	//NOTE: whose seat it is, not which mode - a client must not wire the mirrored tank to its keyboard
	const InputChannel channel{_gameConfig.IsOwnSlot(slot) ? LocalInput(slot) : RemoteInput(slot)};

	return std::make_unique<InputProviderForPlayer>(_events, channel);
}

void TankSpawner::DelayedSpawnStart(const ObjRectangle rect, const int health, const double speed, const Uuid uuid,
									const TankType type)
{
	_delayedSpawns.push_back(DelayedTankSpawn{.uuid = uuid,
											  .type = type,
											  .rect = rect,
											  .health = health,
											  .speed = speed});

	_events->EmitEvent(TankSpawnEvent{.uuid = uuid});

	//NOTE: the burst is also the countdown - the tank lands when its last frame is done; a client's never
	//ends by itself, so it lasts exactly as long as the host's
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = rect, .uuid = uuid, .isEndless = IsClient(_gameMode)});
}

//NOTE: a seat keeps its uuid for the match, so an entry left pending would shadow its next spawn
void TankSpawner::DropDelayedSpawn(const Uuid uuid)
{
	const auto it{std::ranges::find(_delayedSpawns, uuid, &DelayedTankSpawn::uuid)};
	if (it == _delayedSpawns.end())
	{
		return;
	}

	_delayedSpawns.erase(it);
	_events->EmitEvent(AnimationCancelTankSpawnEvent{.uuid = uuid});
}

void TankSpawner::OnSpawnDelayFinished(const Uuid uuid)
{
	const auto it{std::ranges::find(_delayedSpawns, uuid, &DelayedTankSpawn::uuid)};
	if (it == _delayedSpawns.end())
	{
		return;
	}

	DelayedSpawnWith(*it);
	_delayedSpawns.erase(it);
}

void TankSpawner::CancelDelayedSpawnsOf(const Faction faction)
{
	const auto isTargeted = [faction](const DelayedTankSpawn& spawn)
	{
		return (spawn.type > TankType::ENEMY4 ? Faction::PlayerTeam : Faction::EnemyTeam) == faction;
	};

	std::vector<DelayedTankSpawn> cancelled;
	std::ranges::copy_if(_delayedSpawns, std::back_inserter(cancelled), isTargeted);
	std::erase_if(_delayedSpawns, isTargeted);

	for (const DelayedTankSpawn& spawn: cancelled)
	{
		_events->EmitEvent(AnimationCancelTankSpawnEvent{.uuid = spawn.uuid});
		_events->EmitEvent(TankDiedEvent{.who = SeatOf(spawn.type), .uuid = spawn.uuid, .author = Author::None});
	}
}

void TankSpawner::DelayedSpawnWith(const DelayedTankSpawn& params) const
{
	const TankResetProperty resetProperty{.uuid = params.uuid,
										  .rect = params.rect,
										  .health = params.health,
										  .speed = params.speed,
										  .type = params.type,
										  .dir = Direction::UP};

	if (const std::shared_ptr<BaseObj> tank{
			_tankPool->SpawnTank(resetProperty, MakeDriver(params.type))})
	{
		_events->EmitEvent(AddToSpawnQueueEvent{.obj = tank});
		_events->EmitEvent(AnimationCreateTankMoveEvent{.rect = params.rect, .author = SeatOf(params.type)});

		//NOTE: ahead of the effects - their status travels as its own command, and the client has to
		//have built the tank before one arrives for it
		if (IsHost(_gameMode))
		{
			_events->EmitEvent(TankSpawnCompletedEvent{.uuid = params.uuid});
		}

		_events->EmitEvent(BonusReApplyEvent{.uuid = params.uuid, .author = SeatOf(params.type)});
	}
}

//NOTE: a tank already on the host's field lands at once - its burst is long over there
void TankSpawner::RestoreTank(const TankSnapshot& tank) const
{
	const double tankSize{_gameConfig.tankSize};
	const ObjRectangle rect{.x = tank.pos.x, .y = tank.pos.y, .w = tankSize, .h = tankSize};
	DelayedSpawnWith(DelayedTankSpawn{.uuid = tank.uuid,
									  .type = tank.type,
									  .rect = rect,
									  .health = tank.health,
									  .speed = _gameConfig.tankSpeed});

	const Author seat{SeatOf(tank.type)};
	_events->EmitEvent(Key(tank.uuid), PosChangedEvent{.pos = tank.pos, .dir = tank.dir, .uuid = tank.uuid});
	_events->EmitEvent(Key(tank.uuid), TierChangedEvent{.tier = tank.tier, .uuid = tank.uuid});
	//NOTE: always sent - a landing tank switches its helmet on, and only this says whether it still wears it
	_events->EmitEvent(Key(seat), BonusHelmetAppliedEvent{.author = seat, .isActive = tank.isHelmetActive});

	if (tank.isShipActive)
	{
		_events->EmitEvent(Key(seat), BonusShipAppliedEvent{.author = seat});
	}
}
