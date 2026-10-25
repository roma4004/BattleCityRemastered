#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/BonusSpawner.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "entities/pawns/Tank.h"
#include "entities/bonuses/Bonus.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "enums/TankModel.h"
#include "enums/TankType.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <tuple>
#include <vector>

// who is owed a respawn and how many lives are left: a death is announced and the counter of that seat is read
class RespawnManagerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::optional<GameState> _result{};
	//NOTE: the last count each group was told
	std::map<RespawnGroup, unsigned short> _lives{};
	//NOTE: the tanks RespawnManager asked for since the test last cleared it
	std::map<TankType, Uuid> _tanksAsked{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_subs.push_back(TestUtils::WireSpawnQueue(_events, _allObjects));
		_allObjects.reserve(6u);
		const auto bulletPool{std::make_shared<BulletPool>(_events, _allObjects, _gameConfig)};
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		ApplyGameMode(_gameConfig.gameMode);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_subs.push_back(_events->AddListener([this](const GameFinishedEvent& event) { _result = event.state; }));
		_subs.push_back(_events->AddListener([this](const RespawnCountChangedToEvent& event)
		{
			_lives.insert_or_assign(event.group, event.respawnCount);
		}));
		_subs.push_back(_events->AddListener([this](const RespawnTankEvent& event)
		{
			_tanksAsked.insert_or_assign(event.type, event.uuid);
		}));
	}

	void ApplyGameMode(const GameMode mode)
	{
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, mode, _respawnManager, _tankSpawner);
	}

	//NOTE: the first tank of the side dies and leaves the field - one death at a time, as a match has them
	void KillOne(const Faction side)
	{
		const auto tank{std::ranges::find(_allObjects, side, &BaseObj::GetFaction)};
		ASSERT_NE(tank, _allObjects.end()) << "nobody of that side on the field";

		_events->EmitEvent(TankDiedEvent{.uuid = (*tank)->GetUuid()});
		_allObjects.erase(tank);
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

// a map that says how many enemies it has sets the count, whatever its list holds - one that does not keeps twenty
TEST_F(RespawnManagerTest, TheMapsEnemyCountOrTwenty)
{
	struct Lineup final
	{
		EnemyLineupLoadedEvent loaded;
		unsigned short enemies{};
	};

	const std::array lineups{Lineup{.loaded = {.count = 3u, .models = {TankModel::Armor}}, .enemies = 3u},
							 Lineup{.loaded = {}, .enemies = 20u}};
	for (const Lineup& lineup: lineups)
	{
		SCOPED_TRACE(lineup.enemies);
		_events->EmitEvent(GameResetEvent{});

		_events->EmitEvent(lineup.loaded);

		EXPECT_EQ(_lives.at(RespawnGroup::ENEMY_ALL), lineup.enemies);
	}
}

// fewer enemies than seats put no more tanks on the field than there are
TEST_F(RespawnManagerTest, FewerEnemiesThanSeatsFillNoMoreSeatsThanThereAre)
{
	_events->EmitEvent(EnemyLineupLoadedEvent{.count = 2u});
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(std::ranges::count_if(_tanksAsked | std::views::keys, [](const TankType type) { return !SlotOf(type); }),
			  2);
}

// a death takes one off its own group - the enemy team's count, or the seat's
TEST_F(RespawnManagerTest, ADeathTakesALifeOffItsGroup)
{
	constexpr std::array kGroups{std::tuple{RespawnGroup::ENEMY_ALL, GameMode::OnePlayer, 20u},
								 std::tuple{RespawnGroup::PLAYER1, GameMode::OnePlayer, 3u},
								 std::tuple{RespawnGroup::PLAYER2, GameMode::TwoPlayers, 3u}};
	for (const auto& [group, mode, lives]: kGroups)
	{
		SCOPED_TRACE(static_cast<int>(group));
		_allObjects.clear();
		ApplyGameMode(mode);
		_events->EmitEvent(GameResetEvent{});

		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();

		EXPECT_GT(lives, _lives.at(group));
	}
}

// dying past the last life leaves the count at nothing rather than wrapping around
TEST_F(RespawnManagerTest, RunningOutOfLivesStopsAtNothing)
{
	constexpr std::array kGroups{std::tuple{RespawnGroup::ENEMY_ALL, GameMode::OnePlayer, 20u},
								 std::tuple{RespawnGroup::PLAYER1, GameMode::OnePlayer, 3u},
								 std::tuple{RespawnGroup::PLAYER2, GameMode::TwoPlayers, 3u}};
	for (const auto& [group, mode, lives]: kGroups)
	{
		SCOPED_TRACE(static_cast<int>(group));
		_allObjects.clear();
		ApplyGameMode(mode);
		_events->EmitEvent(GameResetEvent{});

		for (unsigned int death{}; death <= lives; ++death)
		{
			_events->EmitEvent(RespawnTanksEvent{});
			if (!_allObjects.empty())
			{
				_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
				_allObjects.pop_back();
			}
		}

		EXPECT_EQ(_lives.at(group), 0u);
	}
}

// the enemy pool emptied wins the match: twenty enemies, and one more if an enemy took an extra life
TEST_F(RespawnManagerTest, AnEmptiedEnemyPoolWins)
{
	struct Pool final
	{
		bool extraLife{};
		int kills{};
	};

	for (const Pool& pool: {Pool{.extraLife = false, .kills = 20}, Pool{.extraLife = true, .kills = 21}})
	{
		SCOPED_TRACE(pool.kills);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		_result.reset();
		if (pool.extraLife)
		{
			TakeAnExtraLife(Author::Enemy1);
		}

		//NOTE: the field is topped up before every death, so each one is a tank the match really put down
		for (int kill{}; kill < pool.kills; ++kill)
		{
			ASSERT_FALSE(_result.has_value()) << "won with enemies left";
			_events->EmitEvent(RespawnTanksEvent{});
			KillOne(Faction::EnemyTeam);
		}

		EXPECT_EQ(_result, GameState::Won);
	}
}

// the player's last life spent loses the match - three of them
TEST_F(RespawnManagerTest, ThePlayersLastLifeSpentLoses)
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
TEST_F(RespawnManagerTest, AFallenEagleLosesWithTheNextDeath)
{
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(PlayersBaseFinishedEvent{});
	ASSERT_FALSE(_result.has_value()) << "the match ended before the player's tank was gone";

	KillOne(Faction::PlayerTeam);

	EXPECT_EQ(_result, GameState::Over);
}

// a life picked up after the eagle fell allows exactly one more death, not a return to three
TEST_F(RespawnManagerTest, ALifeTakenAfterTheEagleFellBuysOneMoreDeath)
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

// 2P free-for-all is won by the last player standing, not by clearing the bots
TEST_F(RespawnManagerTest, TwoPlayersFreeForAllIsWonByTheLastPlayerStanding)
{
	ApplyGameMode(GameMode::TwoPlayersFreeForAll);
	_events->EmitEvent(GameResetEvent{});

	const auto killEvery = [this](const auto& isTarget)
	{
		for (const std::shared_ptr<BaseObj>& tank: _allObjects | std::views::filter(isTarget))
		{
			_events->EmitEvent(TankDiedEvent{.uuid = tank->GetUuid()});
		}

		std::erase_if(_allObjects, isTarget);
	};
	const auto isBot = [](const std::shared_ptr<BaseObj>& obj)
	{
		const auto tank{std::dynamic_pointer_cast<Tank>(obj)};

		return tank != nullptr && FactionOf(tank->GetAuthor()) == Faction::EnemyTeam;
	};
	const auto isPlayerOne = [](const std::shared_ptr<BaseObj>& obj)
	{
		const auto tank{std::dynamic_pointer_cast<Tank>(obj)};

		return tank != nullptr && tank->GetAuthor() == Author::Player1;
	};

	for (int wave{}; wave < 20; ++wave)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		killEvery(isBot);
	}

	ASSERT_FALSE(_result.has_value()) << "the bots were cleared with both players standing, and that ended it";

	for (int life{}; life < 3; ++life)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		killEvery(isPlayerOne);
	}

	EXPECT_EQ(_result, GameState::Won);
}

// an empty seat of a network match asks for no tank until a player sits down in it
TEST_F(RespawnManagerTest, AnEmptySeatSpawnsNothingUntilAPlayerTakesIt)
{
	std::vector<TankType> asked{};
	const EventSubscription respawnSub{_events->AddListener([&asked](const RespawnTankEvent& event)
	{
		if (SlotOf(event.type))
		{
			asked.push_back(event.type);
		}
	})};
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});

	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_EQ(asked, std::vector{TankType::PLAYER1}) << "an empty seat asked for a tank";

	asked.clear();
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Empty});
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(asked, std::vector{TankType::PLAYER2});
}

//NOTE: the bot's losses are not the newcomer's - a seat it left with no lives would give the player nothing to drive
TEST_F(RespawnManagerTest, ASeatWhoseBotSpentEveryLifeIsGivenTheStartingLivesBack)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Bot, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});

	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		_tanksAsked.clear();
		_events->EmitEvent(RespawnTanksEvent{});
		ASSERT_TRUE(_tanksAsked.contains(TankType::PLAYER2)) << "the bot's seat asked for no tank with a life left";
		_events->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = _tanksAsked.at(TankType::PLAYER2)});
	}
	_tanksAsked.clear();
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_FALSE(_tanksAsked.contains(TankType::PLAYER2))
			<< "the control failed - a seat with no lives left asked for a tank";

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Bot});
	EXPECT_EQ(_lives.at(RespawnGroup::PLAYER2), 3u);

	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_TRUE(_tanksAsked.contains(TankType::PLAYER2)) << "the newcomer was given no tank";
	EXPECT_EQ(_lives.at(RespawnGroup::PLAYER2), 2u) << "coming in spent a life the seat never got back";
}

// a client counts a bot's spawn off the seat it sits in, the way the host does
TEST_F(RespawnManagerTest, AClientCountsABotsSpawnOffItsSeat)
{
	ApplyGameMode(GameMode::PlayAsClient);
	_events->EmitEvent(GameResetEvent{});
	ASSERT_EQ(_lives.at(RespawnGroup::PLAYER2), 3u);

	_events->EmitEvent(TankRespawnedEvent{.type = TankType::COOP2,
										  .model = TankModel::Player,
										  .uuid = UuidUtils::GetRandomUuid(),
										  .pos = {}});

	EXPECT_EQ(_lives.at(RespawnGroup::PLAYER2), 2u);
}

//NOTE: taken off, not killed - the life of the tank on the field goes back to the seat for the one who comes back
TEST_F(RespawnManagerTest, ASeatGivenUpGetsTheLifeOfItsTankBack)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_EQ(_lives.at(RespawnGroup::PLAYER2), 2u);

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	EXPECT_EQ(_lives.at(RespawnGroup::PLAYER2), 3u);
}

//NOTE: given up between its tanks - the seat owed a respawn asks for none until its player is back, then gets one
TEST_F(RespawnManagerTest, ASeatGivenUpSpawnsNothingUntilItsPlayerIsBack)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_TRUE(_tanksAsked.contains(TankType::PLAYER2));
	_events->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = _tanksAsked.at(TankType::PLAYER2)});
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});
	_tanksAsked.clear();

	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_TRUE(_tanksAsked.empty()) << "the seat given up asked for a tank";

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Empty});
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_EQ(_tanksAsked.size(), 1u);
	EXPECT_TRUE(_tanksAsked.contains(TankType::PLAYER2));
}

//NOTE: the one who left was the last on the field - taking that tank off leaves nobody to play the match
TEST_F(RespawnManagerTest, TakingOffTheLastTankOnTheFieldLosesTheMatch)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		_tanksAsked.clear();
		_events->EmitEvent(RespawnTanksEvent{});
		ASSERT_TRUE(_tanksAsked.contains(TankType::PLAYER1));
		_events->EmitEvent(TankDiedEvent{.who = Author::Player1, .uuid = _tanksAsked.at(TankType::PLAYER1)});
	}
	ASSERT_FALSE(_result.has_value()) << "the control failed - the match was over with a tank still on the field";

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	EXPECT_EQ(_result, GameState::Over);
}

// a free-for-all whose enemies are gone is won by the one left alone on the field
TEST_F(RespawnManagerTest, InAFreeForAllTheOneLeftAloneWins)
{
	_gameConfig.networkRules = MatchRules::FreeForAll;
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(EnemyLineupLoadedEvent{.count = 0u});
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_FALSE(_result.has_value());

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	EXPECT_EQ(_result, GameState::Won);
}

// with no enemies the map's list never comes - the last player standing wins
TEST_F(RespawnManagerTest, AFreeForAllWithNoEnemiesIsWonByTheLastPlayerStanding)
{
	_gameConfig.networkRules = MatchRules::FreeForAll;
	_gameConfig.simultaneousEnemies = 0u;
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(EnemyLineupLoadedEvent{.count = 20u});

	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		_tanksAsked.clear();
		_events->EmitEvent(RespawnTanksEvent{});
		ASSERT_TRUE(_tanksAsked.contains(TankType::PLAYER2));
		_events->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = _tanksAsked.at(TankType::PLAYER2)});
	}

	EXPECT_EQ(_result, GameState::Won);
}

//NOTE: the starting lives come back only for a bot's losses - a player who left with none comes back with none
TEST_F(RespawnManagerTest, APlayerWhoLeftWithNoLivesComesBackWithNone)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		_tanksAsked.clear();
		_events->EmitEvent(RespawnTanksEvent{});
		ASSERT_TRUE(_tanksAsked.contains(TankType::PLAYER2));
		_events->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = _tanksAsked.at(TankType::PLAYER2)});
	}
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Empty});
	_tanksAsked.clear();

	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_FALSE(_tanksAsked.contains(TankType::PLAYER2)) << "coming back gave a seat with no lives left a tank";
}

//NOTE: the tank taken off is no spawn the match still waits to see die - the last death of the rest loses it
TEST_F(RespawnManagerTest, AfterASeatIsGivenUpTheLastDeathOfTheRestLosesTheMatch)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		ASSERT_TRUE(_tanksAsked.contains(TankType::PLAYER1));
		_events->EmitEvent(TankDiedEvent{.who = Author::Player1, .uuid = _tanksAsked.at(TankType::PLAYER1)});
		_tanksAsked.clear();
		_events->EmitEvent(RespawnTanksEvent{});
	}

	EXPECT_EQ(_result, GameState::Over);
}
