#include "components/GameStatistics.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/StatisticsData.h"
#include "components/WorldSnapshot.h"
#include "enums/Author.h"
#include "enums/Faction.h"
#include "enums/PlayerSlot.h"
#include <optional>

namespace
{
using SeatField = unsigned short SeatStatistics::*;
using TeamField = unsigned short EnemyTeamStatistics::*;

[[nodiscard]] constexpr bool IsEnemyTeam(const Author author) noexcept
{
	return FactionOf(author) == Faction::EnemyTeam;
}

[[nodiscard]] constexpr bool IsFriendlyFire(const Author author, const Author who) noexcept
{
	return IsEnemyTeam(author) == IsEnemyTeam(who);
}
}//namespace

GameStatistics::GameStatistics(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

void GameStatistics::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnGameReset));

	//NOTE: one set for both roles - the host reaches these off its own game logic, the client off the
	//same events re-emitted from the wire, so nothing below asks which one it is
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBulletHit));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnTankHit));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnTankDied));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBrickWallDied));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnSteelWallDied));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBonusPickup));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBonusDestroyed));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBonusExpired));

	_subs.push_back(_events->AddListener(this, &GameStatistics::OnWorldSnapshotRequested));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnWorldSnapshotReceived));
}

void GameStatistics::OnGameReset(const GameResetEvent&) { Reset(); }

//NOTE: the enemy team as one, each seat on its own
void GameStatistics::Credit(const Author author, const TeamField team, const SeatField seat)
{
	if (IsEnemyTeam(author))
	{
		++(_data.enemyTeam.*team);

		return;
	}

	CreditSeat(author, seat);
}

void GameStatistics::CreditSeat(const Author author, const SeatField seat)
{
	if (const std::optional<PlayerSlot> slot{SlotOf(author)})
	{
		++(_data.seats[SeatIndex(*slot)].*seat);
	}
}

void GameStatistics::OnBulletHit(const StatisticsBulletHitEvent& event)
{
	Credit(event.author, &EnemyTeamStatistics::bulletHits, &SeatStatistics::bulletHits);
}

//NOTE: friendly fire is the one hit's to take; across the sides the player scores, hitting or hit
void GameStatistics::OnTankHit(const StatisticsTankHitEvent& event)
{
	if (FactionOf(event.author) == Faction::Neutral)
	{
		return;
	}

	if (IsFriendlyFire(event.author, event.who))
	{
		Credit(event.who, &EnemyTeamStatistics::friendlyHitsTaken, &SeatStatistics::friendlyHitsTaken);

		return;
	}

	CreditSeat(event.author, &SeatStatistics::enemyHits);
	CreditSeat(event.who, &SeatStatistics::hitByEnemyTeam);
}

//NOTE: friendly fire is the one killed's to take; a kill across the sides is the killer's
void GameStatistics::OnTankDied(const TankDiedEvent& event)
{
	if (FactionOf(event.author) == Faction::Neutral)
	{
		return;
	}

	if (IsFriendlyFire(event.author, event.who))
	{
		Credit(event.who, &EnemyTeamStatistics::friendlyKillsTaken, &SeatStatistics::friendlyKillsTaken);

		return;
	}

	Credit(event.author, &EnemyTeamStatistics::playerKills, &SeatStatistics::enemyKills);
}

void GameStatistics::OnBrickWallDied(const BrickWallDiedEvent& event)
{
	Credit(event.author, &EnemyTeamStatistics::brickWallKills, &SeatStatistics::brickWallKills);
}

void GameStatistics::OnSteelWallDied(const SteelWallDiedEvent& event)
{
	Credit(event.author, &EnemyTeamStatistics::steelWallKills, &SeatStatistics::steelWallKills);
}

void GameStatistics::OnBonusPickup(const StatisticsBonusPickupEvent& event)
{
	Credit(event.author, &EnemyTeamStatistics::bonusPickups, &SeatStatistics::bonusPickups);
}

void GameStatistics::OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event)
{
	Credit(event.author, &EnemyTeamStatistics::bonusesDestroyed, &SeatStatistics::bonusesDestroyed);
}

void GameStatistics::OnBonusExpired(const StatisticsBonusExpiredEvent&)
{
	++_data.bonusExpired;
}

void GameStatistics::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	event.snapshot.statistics = _data;
}

void GameStatistics::OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event)
{
	_data = event.snapshot.statistics;
}

void GameStatistics::Reset() { _data = {}; }
