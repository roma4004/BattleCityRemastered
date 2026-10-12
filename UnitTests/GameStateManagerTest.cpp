#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/bonuses/Bonus.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

// a one-player match played out by hand: tanks are asked for, the chosen ones die, and the result is read
class GameStateManagerTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::optional<GameState> _result{};
	EventSubscription _resultSub{};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager,
								 _tankSpawner);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_resultSub = _events->AddListener([this](const GameFinishedEvent& event) { _result = event.state; });
	}

	//NOTE: the first tank of the side dies and leaves the field - one death at a time, as a match has them
	void KillOne(const Faction side)
	{
		const auto tank{std::ranges::find(_allObjects, side, &BaseObj::GetFaction)};
		ASSERT_NE(tank, _allObjects.end()) << "nobody of that side on the field";

		_events->EmitEvent(TankDiedEvent{.uuid = (*tank)->GetUuid()});
		_allObjects.erase(tank);
	}

	//NOTE: the field is topped up before every death, so each one is a tank the match really put down
	int KillEnemiesUntilTheMatchEnds()
	{
		int kills{};
		for (; kills < 40 && !_result; ++kills)
		{
			_events->EmitEvent(RespawnTanksEvent{});
			KillOne(Faction::EnemyTeam);
		}

		return kills;
	}

	void TakeAnExtraLife(const Author author)
	{
		_bonusSpawner->SpawnBonus({.w = _gameConfig.tankSize, .h = _gameConfig.tankSize}, BonusType::Tank);
		const auto bonus{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
		ASSERT_NE(bonus, nullptr);
		bonus->PickUpBonus(author);
		_allObjects.pop_back();
	}
};

// the enemy pool emptied wins the match: twenty enemies, twenty deaths
TEST_F(GameStateManagerTest, AnEmptiedEnemyPoolWins)
{
	EXPECT_EQ(KillEnemiesUntilTheMatchEnds(), 20);
	EXPECT_EQ(_result, GameState::Won);
}

// an enemy's extra life puts one more tank in the pool, and the win waits for it
TEST_F(GameStateManagerTest, AnEnemysExtraLifeDelaysTheWin)
{
	TakeAnExtraLife(Author::Enemy1);

	EXPECT_EQ(KillEnemiesUntilTheMatchEnds(), 21);
	EXPECT_EQ(_result, GameState::Won);
}

// the player's last life spent loses the match - three of them
TEST_F(GameStateManagerTest, ThePlayersLastLifeSpentLoses)
{
	for (int life{}; life < 3; ++life)
	{
		ASSERT_FALSE(_result.has_value()) << "the match ended with a life left";
		_events->EmitEvent(RespawnTanksEvent{});
		KillOne(Faction::PlayerTeam);
	}

	EXPECT_EQ(_result, GameState::Over);
}

// the eagle falling takes every life left at once, so the player's next death loses the match
TEST_F(GameStateManagerTest, AFallenEagleLosesWithTheNextDeath)
{
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(PlayersBaseFinishedEvent{});
	ASSERT_FALSE(_result.has_value()) << "the match ended before the player's tank was gone";

	KillOne(Faction::PlayerTeam);

	EXPECT_EQ(_result, GameState::Over);
}

// a life picked up after the eagle fell buys exactly one more death, not a return to three
TEST_F(GameStateManagerTest, ALifeTakenAfterTheEagleFellBuysOneMoreDeath)
{
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(PlayersBaseFinishedEvent{});
	TakeAnExtraLife(Author::Player1);

	KillOne(Faction::PlayerTeam);
	ASSERT_FALSE(_result.has_value()) << "the extra life bought nothing";

	_events->EmitEvent(RespawnTanksEvent{});
	KillOne(Faction::PlayerTeam);

	EXPECT_EQ(_result, GameState::Over);
}
