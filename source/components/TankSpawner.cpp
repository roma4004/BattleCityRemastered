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
#include "components/events/TimingEvents.h"
#include "components/WorldGeometry.h"
#include "components/WorldSnapshot.h"
#include "entities/TankModelSpec.h"
#include "entities/pawns/Tank.h"
#include "entities/pawns/TankResetProperty.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/InputChannel.h"
#include "enums/PlayerSlot.h"
#include "enums/TankType.h"
#include "enums/Faction.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/Log.h"
#include "utils/MathUtils.h"
#include "utils/ObjectUtils.h"
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "utils/WorldQuery.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
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

[[nodiscard]] bool IsInsideField(const ObjRectangle& rect, const UPoint battlefieldSize)
{
	return rect.x >= 0.0 && rect.y >= 0.0 && rect.Right() <= static_cast<double>(battlefieldSize.x)
		   && rect.Bottom() <= static_cast<double>(battlefieldSize.y);
}

//NOTE: an even roll over the enemy models - for every enemy past the end of the map's list
[[nodiscard]] TankModel RollEnemyModel()
{
	const std::uniform_int_distribution distModel{kFirstTankModelId, kLastEnemyModelId};

	return static_cast<TankModel>(RandUtils::GetRandNumber(distModel));
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
	_subs.push_back(_events->AddListener(this, &TankSpawner::OnNextLevelRequested));
	//NOTE: a tank is the host's call - the client's burst never finishes and waits for TankSpawnComplete
	if (IsAuthority(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnSpawnAnimationFinished));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnEnemyLineupLoaded));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnPostTickUpdate));
	}

	if (IsClient(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankRespawned));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankSpawnCompleted));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankSpawnMoved));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankDied));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnWorldSnapshotReceived));
	}

	if (IsHost(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnWorldSnapshotRequested));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnSeatsFilled));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnSeatHolderChanged));
	}

	//NOTE: a tank mid-spawn is not an object yet, so the grenade cannot reach it the way it reaches
	//the others - the spawner cancels it on their behalf, and only where the world is decided
	if (IsAuthority(_gameMode))
	{
		for (const Faction faction: {Faction::PlayerTeam, Faction::EnemyTeam, Faction::Solo})
		{
			_subs.push_back(_events->AddListener(Key(faction), [this, faction](const BonusGrenadePickupEvent& event)
			{
				CancelDelayedSpawnsOf(faction, event.spared);
			}));
		}
	}
}

void TankSpawner::OnRespawnTank(const RespawnTankEvent& event) { RespawnTank(event.type, event.uuid); }

void TankSpawner::OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event)
{
	const auto it{std::ranges::find(_delayedSpawns, event.uuid, &DelayedTankSpawn::uuid)};
	if (it == _delayedSpawns.end())
	{
		return;
	}

	//NOTE: nothing held the square during the burst, and two tanks in one are wedged for good - one more pass
	if (const ObjRectangle spot{it->rect}; !WorldQuery::IsSpotFreeOfBlockers(_allObjects, spot))
	{
		_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = spot, .uuid = event.uuid});

		return;
	}

	OnSpawnDelayFinished(event.uuid);
}

void TankSpawner::OnTankRespawned(const TankRespawnedEvent& event)
{
	const double tankSize{_gameConfig.tankSize};
	OnClientRespawn(event.type, event.uuid,
					ObjRectangle{.x = event.pos.x, .y = event.pos.y, .w = tankSize, .h = tankSize}, event.model);
}

void TankSpawner::OnTankSpawnCompleted(const TankSpawnCompletedEvent& event)
{
	OnSpawnDelayFinished(event.uuid);
}

//NOTE: the client shoves nothing itself - it draws the square the host says its burst stands on
void TankSpawner::OnTankSpawnMoved(const TankSpawnMovedEvent& event)
{
	const auto it{std::ranges::find(_delayedSpawns, event.uuid, &DelayedTankSpawn::uuid)};
	if (it == _delayedSpawns.end())
	{
		return;
	}

	MoveSpawnSquare(*it, ObjRectangle{.x = event.pos.x, .y = event.pos.y, .w = it->rect.w, .h = it->rect.h});
}

//NOTE: after everything has moved - the square is shoved by where the hulls ended up this frame
void TankSpawner::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	std::ranges::for_each(_delayedSpawns, [this](DelayedTankSpawn& spawn) { NudgeSpawnSquare(spawn); });
}

void TankSpawner::NudgeSpawnSquare(DelayedTankSpawn& spawn)
{
	if (IsSpawnSpotFree(spawn.home, spawn.uuid))
	{
		MoveSpawnSquare(spawn, spawn.home);

		return;
	}

	const Tank* const pusher{TankStandingIn(spawn.rect)};
	if (pusher == nullptr)
	{
		return;
	}

	if (const std::optional<ObjRectangle> aside{RoomOutOfTheWay(spawn, *pusher)})
	{
		MoveSpawnSquare(spawn, *aside);
	}
}

void TankSpawner::MoveSpawnSquare(DelayedTankSpawn& spawn, const ObjRectangle& to)
{
	if (MathUtils::AreEqualAbsolute(spawn.rect.x, to.x) && MathUtils::AreEqualAbsolute(spawn.rect.y, to.y))
	{
		return;
	}

	spawn.rect = to;
	_events->EmitEvent(AnimationMoveTankSpawnEvent{.rect = to, .uuid = spawn.uuid});

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(TankSpawnMovedEvent{.uuid = spawn.uuid, .pos = FPoint{.x = to.x, .y = to.y}});
	}
}

//NOTE: only a hull shoves - a wall grown on the square or a shell crossing it is waited out
const Tank* TankSpawner::TankStandingIn(const ObjRectangle& rect) const
{
	const auto tankThere = [&rect](const std::shared_ptr<BaseObj>& object) -> const Tank*
	{
		if (!ObjectUtils::IsAlive(object.get()) || !ColliderUtils::IsCollide(rect, object->GetRect()))
		{
			return nullptr;
		}

		return dynamic_cast<const Tank*>(object.get());
	};
	const auto tanks{_allObjects | std::views::transform(tankThere)};
	const auto found{std::ranges::find_if(tanks, [](const Tank* const tank) { return tank != nullptr; })};

	return found == tanks.end() ? nullptr : *found;
}

std::optional<ObjRectangle> TankSpawner::RoomOutOfTheWay(const DelayedTankSpawn& spawn, const Tank& pusher) const
{
	const ObjRectangle hull{pusher.GetRect()};
	const auto clearOf = [&spawn, &hull](const Direction dir)
	{
		const auto [dx, dy]{DirectionUtils::Unit(dir)};
		ObjRectangle out{spawn.rect};
		if (dx != 0.0)
		{
			out.x = dx > 0.0 ? hull.Right() : hull.x - out.w;
		}
		else
		{
			out.y = dy > 0.0 ? hull.Bottom() : hull.y - out.h;
		}

		return out;
	};

	const Direction driving{pusher.GetDirection()};
	const auto [oneSide, otherSide]{DirectionUtils::Laterals(driving)};
	const std::array squares{clearOf(driving), clearOf(oneSide), clearOf(otherSide),
							 clearOf(DirectionUtils::Opposite(driving))};

	const auto isRoom = [this, &spawn](const ObjRectangle& square)
	{
		return IsInsideField(square, _gameConfig.battlefieldSize) && IsSpawnSpotFree(square, spawn.uuid);
	};
	const auto found{std::ranges::find_if(squares, isRoom)};

	return found == squares.end() ? std::nullopt : std::optional{*found};
}

void TankSpawner::OnTankDied(const TankDiedEvent& event) { DropDelayedSpawn(event.uuid); }

void TankSpawner::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	std::ranges::transform(_delayedSpawns, std::back_inserter(event.snapshot.tankSpawns),
						   [](const DelayedTankSpawn& spawn)
						   {
							   return TankRespawnedEvent{.type = spawn.type,
														 .model = spawn.model,
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

void TankSpawner::OnSeatsFilled(const SeatsFilledEvent& event)
{
	for (const PlayerSlot slot: kSlots)
	{
		_botSeats.set(SeatIndex(slot), event.holders[SeatIndex(slot)] == SeatHolder::Bot);
	}

	//NOTE: the one who earned it is not in - whoever takes the seat later starts afresh, not on a stranger's tier
	std::erase_if(_nextLevelLoadouts, [&event](const NextLevelLoadout& loadout)
	{
		return event.holders[SeatIndex(*SlotOf(loadout.type))] != SeatHolder::Player;
	});
}

//NOTE: the tank changes hands where it stands, its health and tier with it - from the bot or to the bot
void TankSpawner::OnSeatHolderChanged(const SeatHolderChangedEvent& event)
{
	const bool isBotTaking{event.to == SeatHolder::Bot};
	_botSeats.set(SeatIndex(event.slot), isBotTaking);
	if (event.from == SeatHolder::Empty || event.from == event.to)
	{
		return;
	}

	const TankType from{event.from == SeatHolder::Bot ? CoopTankOf(event.slot) : PlayerTankOf(event.slot)};
	const auto leftTank = [from](const std::shared_ptr<BaseObj>& object) -> Tank*
	{
		auto* const tank{dynamic_cast<Tank*>(object.get())};

		return tank != nullptr && ObjectUtils::IsAlive(tank) && tank->_type == from ? tank : nullptr;
	};
	const auto tanks{_allObjects | std::views::transform(leftTank)};
	const auto found{std::ranges::find_if(tanks, [](const Tank* const tank) { return tank != nullptr; })};
	Tank* const standing{found == tanks.end() ? nullptr : *found};

	//NOTE: given up to nobody - off the field without a death, so nobody scores it and nothing bursts
	if (event.to == SeatHolder::Empty)
	{
		if (const auto spawn{std::ranges::find(_delayedSpawns, from, &DelayedTankSpawn::type)};
			spawn != _delayedSpawns.end())
		{
			DropDelayedSpawn(spawn->uuid);
		}

		if (standing != nullptr)
		{
			standing->SetIsAlive(false);
		}

		return;
	}

	const TankType to{isBotTaking ? CoopTankOf(event.slot) : PlayerTankOf(event.slot)};
	const auto isLeft = [from](const DelayedTankSpawn& spawn) { return spawn.type == from; };
	std::ranges::for_each(_delayedSpawns | std::views::filter(isLeft),
						  [to](DelayedTankSpawn& spawn) { spawn.type = to; });

	if (standing != nullptr)
	{
		standing->Handover(to, MakeDriver(to));
	}
}

void TankSpawner::Reset(const GameResetEvent& event)
{
	_enemySpawnTimer.cooldown = _gameConfig.enemySpawnCooldown;
	_enemySpawnTimer.isActive = false;
	_enemySpawnTimer.activateTime = TimeUtils::Now() - _enemySpawnTimer.cooldown;
	_delayedSpawns.clear();

	if (!event.keepsPlayerProgress)
	{
		_nextLevelLoadouts.clear();
	}
}

void TankSpawner::OnEnemyLineupLoaded(const EnemyLineupLoadedEvent& event)
{
	_enemyLineup = event.models;
	_enemiesChosen = 0u;
}

TankModel TankSpawner::NextEnemyModel()
{
	const std::size_t next{_enemiesChosen++};

	return next < _enemyLineup.size() ? _enemyLineup[next] : RollEnemyModel();
}

//NOTE: asked of the field while it still stands - the reset that empties it comes a phase later, and
//on a server several ready signals later
void TankSpawner::OnNextLevelRequested(const NextLevelRequestedEvent&)
{
	if (IsClient(_gameMode))
	{
		return;
	}

	WorldSnapshot field{};
	_events->EmitEvent(WorldSnapshotRequestedEvent{.snapshot = field});

	_nextLevelLoadouts.clear();
	for (const TankSnapshot& tank: field.tanks)
	{
		if (IsPlayerTank(tank.type))
		{
			_nextLevelLoadouts.push_back(NextLevelLoadout{.type = tank.type,
											   .tier = tank.tier,
											   .health = tank.health,
											   .isShipActive = tank.isShipActive});
		}
	}
}

TankSpawner::NextLevelLoadout TankSpawner::LoadoutOf(const TankType type) const
{
	const auto found{std::ranges::find(_nextLevelLoadouts, type, &NextLevelLoadout::type)};

	return found == _nextLevelLoadouts.end() ? NextLevelLoadout{.type = type} : *found;
}

//NOTE: the tier and the health ride in with the reset property, the ship is a pickup nobody picked up - and
//the client hears none of it, so the host says it out loud once the tank is there
void TankSpawner::SpendLoadout(const Uuid uuid, const TankType type, const int health)
{
	const auto found{std::ranges::find(_nextLevelLoadouts, type, &NextLevelLoadout::type)};
	if (found == _nextLevelLoadouts.end())
	{
		return;
	}

	const NextLevelLoadout carried{*found};
	_nextLevelLoadouts.erase(found);

	if (carried.isShipActive)
	{
		_events->EmitEvent(Key(SeatOf(type)), BonusShipPickupEvent{});
	}

	if (!IsHost(_gameMode))
	{
		return;
	}

	if (carried.tier > 1u)
	{
		_events->EmitEvent(TierChangedEvent{.tier = carried.tier, .uuid = uuid});
	}

	_events->EmitEvent(HealthChangedEvent{.health = health, .uuid = uuid});
}

bool TankSpawner::IsSpawnSpotFree(const ObjRectangle& rect, const Uuid ignoredSpawn) const
{
	const auto isBurstThere = [&rect, ignoredSpawn](const DelayedTankSpawn& pending)
	{
		return pending.uuid != ignoredSpawn && ColliderUtils::IsCollide(rect, pending.rect);
	};

	return WorldQuery::IsSpotFreeOfBlockers(_allObjects, rect) && std::ranges::none_of(_delayedSpawns, isBurstThere);
}

std::optional<ObjRectangle> TankSpawner::FindSpawnSpot(const double minX, const double maxX, const double y,
													   const double preferredX) const
{
	const double tankSize{_gameConfig.tankSize};
	auto isFree = [this, y, tankSize](const double x)
	{
		return IsSpawnSpotFree(ObjRectangle{.x = x, .y = y, .w = tankSize, .h = tankSize});
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
	const auto battleFieldSizeX{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const double quarterWidth{battleFieldSizeX / 4.0};
	const double lastX{battleFieldSizeX - tankSize};

	//NOTE: the whole hull inside a seat's quarter, so no two seats overlap; too narrow a field falls back to the front
	const auto quarter{static_cast<double>(type)};
	const double minX{quarter * quarterWidth};
	const double maxX{std::max(minX, minX + quarterWidth - tankSize)};

	const std::uniform_real_distribution<double> distRandX{minX, maxX};
	const double randomX{RandUtils::GetRandNumber(distRandX)};

	//NOTE: the whole front only once its own quarter is full - no tank at all is worse than one out of place
	return FindSpawnSpot(minX, maxX, 0.0, randomX)
		   .or_else([this, lastX, randomX] { return FindSpawnSpot(0.0, lastX, 0.0, randomX); });
}

bool TankSpawner::SpawnEnemy(const ObjRectangle rect, const Uuid uuid, const TankType type, const TankModel model)
{
	const std::string name{"Enemy" + std::to_string(static_cast<int>(type) + 1)};
	const Faction faction{FactionOf(SeatOf(type), _gameConfig.Rules())};

	Log::Info("spawn " + name + " (" + std::string{ToString(faction)} + ") " + std::string{ToString(model)}
			  + " uuid " + UuidUtils::GetStringUuid(uuid));

	DelayedSpawnStart(rect, uuid, type, model);

	return true;
}

//NOTE: a bot in a player's seat is still the player's tank - the seat says who drives, the model what it is worth
void TankSpawner::SpawnPlayer(const ObjRectangle rect, const Uuid uuid, const TankType type)
{
	const Faction faction{FactionOf(SeatOf(type), _gameConfig.Rules())};

	const std::string name{(IsPlayerTank(type) ? "Player" : "CoopBot") + std::to_string(SeatIndex(*SlotOf(type)) + 1u)};
	Log::Info("spawn " + name + " (" + std::string{ToString(faction)} + ") uuid " + UuidUtils::GetStringUuid(uuid));

	DelayedSpawnStart(rect, uuid, type, TankModel::Player);
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const Uuid uuid, const std::optional<ObjRectangle> rect,
									const std::optional<TankModel> model)
{
	const std::optional<ObjRectangle> spawnRect{rect.has_value() ? rect : GetEnemyRandomPosX(type)};
	if (!spawnRect)
	{
		return;
	}

	const TankModel spawnModel{model.has_value() ? *model : NextEnemyModel()};
	const bool isSuccessSpawn{SpawnEnemy(*spawnRect, uuid, type, spawnModel)};
	if (isSuccessSpawn && IsHost(_gameMode))
	{
		_events->EmitEvent(TankRespawnedEvent{.type = type,
											  .model = spawnModel,
											  .uuid = uuid,
											  .pos = FPoint{.x = spawnRect->x, .y = spawnRect->y}});
	}
}

std::optional<ObjRectangle> TankSpawner::GetPlayerRandomPosX(const PlayerSlot slot) const
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

	const bool isLeft{slot == PlayerSlot::P1 || slot == PlayerSlot::P3};
	auto [minX, maxX]{isLeft ? std::pair{0.0, middle - fortressHalfWidth - tankSize}
							 : std::pair{middle + fortressHalfWidth, battleFieldSizeX - tankSize}};
	//NOTE: past two seats each side splits, the first pair next to the fortress
	if (_gameConfig.SeatCount() > 2u)
	{
		const double split{(minX + maxX) / 2.0};
		const bool isInner{slot == PlayerSlot::P1 || slot == PlayerSlot::P2};
		(isLeft == isInner ? minX : maxX) = split;
	}

	const std::uniform_real_distribution distRandId{minX, maxX};
	const double randomX{RandUtils::GetRandNumber(distRandId)};

	return FindSpawnSpot(minX, maxX, battleFieldSizeY - tankSize, randomX);
}

void TankSpawner::RespawnPlayerTeam(const TankType type, const Uuid uuid,
									const std::optional<ObjRectangle> rect)
{
	const PlayerSlot slot{*SlotOf(type)};
	const std::optional<ObjRectangle> spawnRect{rect.has_value() ? rect : GetPlayerRandomPosX(slot)};
	if (!spawnRect)
	{
		return;
	}

	//NOTE: asked for as the player's seat - who drives it is the authority's call, a client spawns what it is told
	const TankType spawned{IsAuthority(_gameMode) && IsBotSeat(slot) ? CoopTankOf(slot) : type};
	SpawnPlayer(*spawnRect, uuid, spawned);
	if (IsHost(_gameMode))
	{
		_events->EmitEvent(TankRespawnedEvent{.type = spawned,
											  .model = TankModel::Player,
											  .uuid = uuid,
											  .pos = FPoint{.x = spawnRect->x, .y = spawnRect->y}});
	}
}

//NOTE: the demo is the phase where nobody sits down - every seat goes to a bot
bool TankSpawner::IsBotSeat(const PlayerSlot slot) const
{
	return _gameConfig.gameState == GameState::Demo || (UsesCoopBots(_gameMode) && slot != PlayerSlot::P1)
		   || _botSeats.test(SeatIndex(slot));
}

void TankSpawner::RespawnTank(const TankType type, const Uuid uuid, const std::optional<ObjRectangle> rect,
							  const std::optional<TankModel> model)
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
				RespawnEnemyTanks(type, uuid, rect, model);
			}
			else if (_enemySpawnTimer.IsCooldownFinish())
			{
				_enemySpawnTimer.Reset();
				RespawnEnemyTanks(type, uuid, rect, model);
			}
			break;
		}
		case TankType::PLAYER1:
		case TankType::PLAYER2:
		case TankType::PLAYER3:
		case TankType::PLAYER4:
		case TankType::COOP1:
		case TankType::COOP2:
		case TankType::COOP3:
		case TankType::COOP4:
			RespawnPlayerTeam(type, uuid, rect);
			break;
	}
}

//TODO: maybe we don't need spawn on client at all and just move the textures and animation?
void TankSpawner::OnClientRespawn(const TankType type, const Uuid uuid, const ObjRectangle rect,
								  const TankModel model)
{
	RespawnTank(type, uuid, rect, model);
}

std::unique_ptr<IInputProvider> TankSpawner::MakeDriver(const TankType type) const
{
	if (!IsPlayerTank(type))
	{
		return std::make_unique<InputProviderForBot>(_allObjects, _gameConfig);
	}

	const PlayerSlot slot{*SlotOf(type)};

	//NOTE: whose seat it is, not which mode - a client must not wire the mirrored tank to its keyboard
	const InputChannel channel{_gameConfig.IsOwnSlot(slot) ? LocalInput(slot) : RemoteInput(slot)};

	return std::make_unique<InputProviderForPlayer>(_events, channel);
}

void TankSpawner::DelayedSpawnStart(const ObjRectangle rect, const Uuid uuid, const TankType type,
									const TankModel model)
{
	const NextLevelLoadout loadout{LoadoutOf(type)};
	//NOTE: a wounded tank starts the level whole, a healed one keeps what it gained above that
	const int health{std::max(loadout.health, HealthOf(model, _gameConfig.tankHealth))};
	_delayedSpawns.push_back(DelayedTankSpawn{.uuid = uuid,
											  .type = type,
											  .model = model,
											  .rect = rect,
											  .home = rect,
											  .health = health,
											  .tier = loadout.tier});

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

	const DelayedTankSpawn spawn{*it};
	_delayedSpawns.erase(it);

	DelayedSpawnWith(spawn);
}

void TankSpawner::CancelDelayedSpawnsOf(const Faction faction, const Author spared)
{
	const auto isTargeted = [this, faction, spared](const DelayedTankSpawn& spawn)
	{
		const Author author{SeatOf(spawn.type)};

		return author != spared && FactionOf(author, _gameConfig.Rules()) == faction;
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

void TankSpawner::DelayedSpawnWith(const DelayedTankSpawn& params)
{
	const TankResetProperty resetProperty{.uuid = params.uuid,
										  .rect = params.rect,
										  .health = params.health,
										  .type = params.type,
										  .model = params.model,
										  .dir = Direction::UP,
										  .tier = params.tier};

	if (const std::shared_ptr<BaseObj> tank{
			_tankPool->SpawnTank(resetProperty, MakeDriver(params.type))})
	{
		_events->EmitEvent(AddToSpawnQueueEvent{.obj = tank});
		_events->EmitEvent(AnimationCreateTankMoveEvent{.rect = params.rect,
														.author = SeatOf(params.type),
														.model = params.model,
														.tier = params.tier});

		//NOTE: ahead of the effects - their status travels as its own command, and the client has to
		//have built the tank before one arrives for it
		if (IsHost(_gameMode))
		{
			_events->EmitEvent(TankSpawnCompletedEvent{.uuid = params.uuid});
		}

		_events->EmitEvent(BonusReApplyEvent{.uuid = params.uuid, .author = SeatOf(params.type)});
		SpendLoadout(params.uuid, params.type, params.health);
	}
}

//NOTE: a tank already on the host's field lands at once - its burst is long over there
void TankSpawner::RestoreTank(const TankSnapshot& tank)
{
	const double tankSize{_gameConfig.tankSize};
	const ObjRectangle rect{.x = tank.pos.x, .y = tank.pos.y, .w = tankSize, .h = tankSize};
	DelayedSpawnWith(DelayedTankSpawn{.uuid = tank.uuid,
									  .type = tank.type,
									  .model = tank.model,
									  .rect = rect,
									  .health = tank.health});

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
