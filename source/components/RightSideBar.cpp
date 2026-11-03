#include "components/RightSideBar.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/ReplicationEvents.h"
#include "entities/EffectTarget.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "enums/UiIcon.h"
#include "network/commands/WorldSnapshot.h"
#include "utils/Timer.h"
#include "utils/TimeUtils.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <ranges>
#include <utility>
#include <variant>
#include <vector>

//NOTE: until a key says otherwise, the keyboard half that drives the seat - a client's third or fourth is on the left
RightSideBar::RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _seatDevices{InputDevice::KeyboardLeft, InputDevice::KeyboardRight, InputDevice::KeyboardLeft,
				   InputDevice::KeyboardLeft}
	, _gameConfig{gameConfig}
	, _events{events}
{
	Subscribe();
}

void RightSideBar::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnGameModeApplied));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnGameReset));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnRespawnCountChangedTo));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnMapLoaded));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnSeatHoldersAnnounced));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnBonusEffectsChanged));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnBonusShipApplied));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnTankDied));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnShareRecipientChanged));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnWorldSnapshotReceived));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnTabReleased));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnSeatDeviceUsed));
}

void RightSideBar::Draw() const
{
	const unsigned short enemyReserve{RespawnCountOf(RespawnGroup::ENEMY_ALL)};
	_events->EmitEvent(RenderSideBarEvent{.enemies = SideBarEnemies{.reserve = enemyReserve,
																	.badges = EnemyBadges(),
																	.teamFrozen = SpentOn(FreezesEnemyTeam)},
										  .players = SideBarPlayers{.seats = Seats(),
																	.stage = _stageNumber,
																	.fortified = SpentOn(FortifiesBase)}});
}

//NOTE: a new badge takes the first empty cell under the reserve; one the reserve grew over is placed anew
void RightSideBar::PlaceEnemyBadges()
{
	const int reserve{std::min(static_cast<int>(RespawnCountOf(RespawnGroup::ENEMY_ALL)), kEnemyFrameCells)};
	std::erase_if(_enemyBadges, [this, reserve](const EnemyBadge& badge)
	{
		return badge.cell < reserve || !EnemyBadgeSpent(badge.seat, badge.type);
	});

	const auto cells{std::views::iota(reserve, kEnemyFrameCells)};
	const auto sharesCell = [](const BonusType type) { return type == BonusType::Helmet || type == BonusType::Ship; };
	for (const auto [type, seat]:
		 std::views::cartesian_product(std::array{BonusType::Helmet, BonusType::Ship, BonusType::Timer}, kEnemyAuthors))
	{
		const bool isPlaced{std::ranges::any_of(_enemyBadges, [type, seat](const EnemyBadge& badge)
		{
			return badge.type == type && badge.seat == seat;
		})};
		if (isPlaced || !EnemyBadgeSpent(seat, type))
		{
			continue;
		}

		//NOTE: an enemy's helmet and ship share a cell - the ship is drawn beside the frame
		const auto mate{std::ranges::find_if(_enemyBadges, [&sharesCell, type, seat](const EnemyBadge& badge)
		{
			return badge.seat == seat && sharesCell(badge.type) && sharesCell(type);
		})};
		if (mate != _enemyBadges.end())
		{
			_enemyBadges.push_back(EnemyBadge{.seat = seat, .type = type, .cell = mate->cell});

			continue;
		}

		const auto empty{std::ranges::find_if(cells, [this](const int cell)
		{
			return !std::ranges::contains(_enemyBadges, cell, &EnemyBadge::cell);
		})};
		if (empty != cells.end())
		{
			_enemyBadges.push_back(EnemyBadge{.seat = seat, .type = type, .cell = *empty});
		}
	}
}

std::vector<SideBarEnemyBadge> RightSideBar::EnemyBadges() const
{
	return _enemyBadges | std::views::transform([this](const EnemyBadge& badge)
	{
		return SideBarEnemyBadge{.cell = badge.cell,
								 .type = badge.type,
								 .spent = EnemyBadgeSpent(badge.seat, badge.type)};
	}) | std::ranges::to<std::vector>();
}

//NOTE: a free-for-all timer freezes every enemy seat but the one that took it
std::optional<float> RightSideBar::EnemyBadgeSpent(const Author seat, const BonusType type) const
{
	if (type == BonusType::Helmet)
	{
		return HelmetSpent(seat);
	}

	if (type == BonusType::Ship)
	{
		return ShipSpent(seat);
	}

	return SpentOn([seat](const RunningEffect& effect)
	{
		return FreezesSeat(effect.type, effect.target, effect.spared, Faction::Solo, seat);
	});
}

std::vector<SideBarSeat> RightSideBar::Seats() const
{
	const auto seatOf = [this](const PlayerSlot slot)
	{
		const Author seat{AuthorOf(slot)};
		const Faction side{FactionOf(seat, _gameConfig.Rules())};

		return SideBarSeat{.lives = LivesOf(slot),
						   .frozen = SpentOn([seat, side](const RunningEffect& effect)
						   {
							   return FreezesSeat(effect.type, effect.target, effect.spared, side, seat);
						   }),
						   .helmet = HelmetSpent(seat),
						   .ship = ShipSpent(seat),
						   .label = LabelOf(slot)};
	};

	return kSlots | std::views::take(_gameConfig.SeatCount()) | std::views::transform(seatOf)
		   | std::ranges::to<std::vector>();
}

//NOTE: an empty seat shows 0 lives; its stock is kept for whoever takes the seat
unsigned short RightSideBar::LivesOf(const PlayerSlot slot) const
{
	if (_holders[SeatIndex(slot)] == SeatHolder::Empty)
	{
		return 0u;
	}

	return RespawnCountOf(GroupOf(slot));
}

unsigned short RightSideBar::RespawnCountOf(const RespawnGroup group) const
{
	return _respawnCounts[static_cast<std::size_t>(group)];
}

std::optional<float> RightSideBar::SpentOn(const std::predicate<const RunningEffect&> auto& isIt) const
{
	const auto it{std::ranges::find_if(_effects, [&isIt](const RunningEffect& effect)
	{
		return IsRunning(effect) && isIt(effect);
	})};

	return it == _effects.end() ? std::nullopt : std::optional{Spent(*it)};
}

std::optional<float> RightSideBar::HelmetSpent(const Author seat) const
{
	return SpentOn([seat](const RunningEffect& effect)
	{
		return effect.type == BonusType::Helmet && effect.target == EffectTarget{seat};
	});
}

//NOTE: spent by charges, not by time - shown whole while it lasts
std::optional<float> RightSideBar::ShipSpent(const Author seat) const
{
	return std::ranges::contains(_ships, seat) ? std::optional{0.f} : std::nullopt;
}

float RightSideBar::Spent(const RunningEffect& effect)
{
	const std::chrono::duration<float> left{effect.timer.TimeLeft(TimeUtils::Now())};
	const std::chrono::duration<float> duration{effect.duration};

	return 1.f - left / duration;
}

bool RightSideBar::IsRunning(const RunningEffect& effect) { return !effect.timer.IsCooldownFinish(); }

bool RightSideBar::FreezesEnemyTeam(const RunningEffect& effect)
{
	return effect.type == BonusType::Timer && effect.target == EffectTarget{Faction::EnemyTeam};
}

bool RightSideBar::FortifiesBase(const RunningEffect& effect)
{
	return effect.type == BonusType::Shovel && effect.target == EffectTarget{Faction::PlayerTeam};
}

//NOTE: any bot's seat is free to join
SideBarLabel RightSideBar::LabelOf(const PlayerSlot slot) const
{
	if (!IsInMatch(_gameConfig.gameState))
	{
		return {};
	}

	const bool isBot{_holders[SeatIndex(slot)] == SeatHolder::Bot};
	if (isBot && _gameConfig.ownSlot == slot)
	{
		return SideBarLabel{.text = "PRESS TO START"};
	}

	return ShareLabelOf(slot).value_or(SideBarLabel{.text = isBot ? "WAITING" : ""});
}

//NOTE: over the seat the life goes to, so the giver's own lives stay in sight; the giver's machine says what to press
std::optional<SideBarLabel> RightSideBar::ShareLabelOf(const PlayerSlot slot) const
{
	if (_gameConfig.gameState != GameState::Playing || _shareRecipient != slot)
	{
		return std::nullopt;
	}

	const auto seats{kSlots | std::views::take(_gameConfig.SeatCount())};
	const auto isOwnGiver = [this](const PlayerSlot seat) { return _gameConfig.IsOwnSlot(seat) && CanShareLife(seat); };
	if (const auto giver{std::ranges::find_if(seats, isOwnGiver)};
		giver != seats.end())
	{
		return GiverLabelOf(*giver);
	}

	return SideBarLabel{.text = "WAITING SHARE"};
}

SideBarLabel RightSideBar::GiverLabelOf(const PlayerSlot giver) const
{
	switch (_seatDevices[SeatIndex(giver)])
	{
		case InputDevice::KeyboardLeft:
			return SideBarLabel{.text = "Q SHARE SPAWN"};
		case InputDevice::KeyboardRight:
			return SideBarLabel{.text = "O SHARE SPAWN"};
		case InputDevice::XBoxPad:
			return SideBarLabel{.text = "SHARE SPAWN", .button = UiIcon::SideBarXBoxX};
		case InputDevice::PlayStationPad:
			return SideBarLabel{.text = "SHARE SPAWN", .button = UiIcon::SideBarPS5Square};
	}

	return {};
}

//NOTE: a bot's seat has nobody to press the key
bool RightSideBar::CanShareLife(const PlayerSlot slot) const
{
	return IsDrivenSeat(_gameConfig.gameMode, _holders[SeatIndex(slot)], slot) && RespawnCountOf(GroupOf(slot)) > 0u;
}

//NOTE: a network seat is empty until the host says who took it
void RightSideBar::OnGameModeApplied(const GameModeAppliedEvent& event)
{
	_holders.fill(IsNetworkGame(event.mode) ? SeatHolder::Empty : SeatHolder::Player);
	_shareRecipient.reset();
}

void RightSideBar::OnGameReset(const GameResetEvent&)
{
	_effects.clear();
	_enemyBadges.clear();
	_ships.clear();
}

void RightSideBar::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	_respawnCounts[static_cast<std::size_t>(event.group)] = event.respawnCount;
	PlaceEnemyBadges();
}

void RightSideBar::OnMapLoaded(const MapLoadedEvent& event) { _stageNumber = event.stage; }

//NOTE: a seat given up takes its tank off the field with no death
void RightSideBar::OnSeatHoldersAnnounced(const SeatHoldersAnnouncedEvent& event)
{
	_holders = event.holders;
	std::erase_if(_ships, [this](const Author seat)
	{
		const std::optional<PlayerSlot> slot{SlotOf(seat)};

		return slot && _holders[SeatIndex(*slot)] == SeatHolder::Empty;
	});
}

void RightSideBar::OnBonusEffectsChanged(const BonusEffectsChangedEvent& event)
{
	const auto toRunning = [](const BonusEffectTimeLeft& entry)
	{
		return RunningEffect{.type = entry.type,
							 .target = entry.target,
							 .spared = entry.spared,
							 .timer = Timer{entry.left},
							 .duration = entry.duration};
	};

	_effects = event.effects | std::views::transform(toRunning) | std::ranges::to<std::vector>();
	PlaceEnemyBadges();
}

void RightSideBar::OnBonusShipApplied(const BonusShipAppliedEvent& event)
{
	std::erase(_ships, event.author);
	if (event.isActive)
	{
		_ships.push_back(event.author);
	}

	PlaceEnemyBadges();
}

void RightSideBar::OnTankDied(const TankDiedEvent& event)
{
	std::erase(_ships, event.who);
	PlaceEnemyBadges();
}

void RightSideBar::OnShareRecipientChanged(const ShareRecipientChangedEvent& event) { _shareRecipient = event.seat; }

void RightSideBar::OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event)
{
	_shareRecipient = event.snapshot.shareRecipient;
}

//NOTE: a pair's devices trade seats, so each player keeps the prompt of the device in hand
void RightSideBar::OnTabReleased(const TabReleasedEvent& event)
{
	//NOTE: a client's third or fourth seat stays with its first device, as UserInput::DeviceSlot has it
	if (event.isSecondPair && _gameConfig.ownSlot && SeatIndex(*_gameConfig.ownSlot) >= 2u)
	{
		return;
	}

	const std::size_t first{event.isSecondPair ? 2u : 0u};
	std::swap(_seatDevices[first], _seatDevices[first + 1u]);
}

void RightSideBar::OnSeatDeviceUsed(const SeatDeviceUsedEvent& event)
{
	_seatDevices[SeatIndex(event.slot)] = event.device;
}
