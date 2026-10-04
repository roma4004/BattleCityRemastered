#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
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
#include <cstddef>
#include <memory>
#include <optional>
#include <ranges>
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
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_allObjects.reserve(6u);
		const auto bulletPool{std::make_shared<BulletPool>(_events, _allObjects, _gameConfig)};
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
	}

	void TearDown() override {}

	//NOTE: the latest count the enemy team was told, read into the given variable
	[[nodiscard]] EventSubscription WatchEnemiesLeft(unsigned short& enemiesLeft) const
	{
		return _events->AddListener([&enemiesLeft](const RespawnCountChangedToEvent& event)
		{
			if (event.group == RespawnGroup::ENEMY_ALL)
			{
				enemiesLeft = event.respawnCount;
			}
		});
	}
};

// a map that says how many enemies it has sets the count, whatever its list holds
TEST_F(RespawnManagerTest, TheMapsEnemyCountSetsTheEnemyCount)
{
	unsigned short enemiesLeft{};
	const EventSubscription countSub{WatchEnemiesLeft(enemiesLeft)};

	_events->EmitEvent(EnemyLineupLoadedEvent{.count = 3u, .models = {TankModel::Armor}});

	EXPECT_EQ(enemiesLeft, 3u);
}

// one that does not say keeps the twenty the reset counted
TEST_F(RespawnManagerTest, AMapWithoutAnEnemyCountSendsTwenty)
{
	unsigned short enemiesLeft{};
	const EventSubscription countSub{WatchEnemiesLeft(enemiesLeft)};

	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(EnemyLineupLoadedEvent{});

	EXPECT_EQ(enemiesLeft, 20u);
}

// fewer enemies than seats put no more tanks on the field than there are
TEST_F(RespawnManagerTest, FewerEnemiesThanSeatsFillNoMoreSeatsThanThereAre)
{
	std::size_t enemiesSent{};
	const EventSubscription respawnSub{_events->AddListener([&enemiesSent](const RespawnTankEvent& event)
	{
		enemiesSent += SlotOf(event.type) ? 0u : 1u;
	})};

	_events->EmitEvent(EnemyLineupLoadedEvent{.count = 2u});
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(enemiesSent, 2u);
}

// a dead bot takes one off the enemy team's count
TEST_F(RespawnManagerTest, EnemyDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

// a dead player one off its own
TEST_F(RespawnManagerTest, PlayerOneDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

// and player two off the second seat's
TEST_F(RespawnManagerTest, PlayerTwoDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

// spending the last enemy life empties the count
TEST_F(RespawnManagerTest, EnemyRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();
	}

	EXPECT_EQ(0u, respawnActual);

}

// the same for player one
TEST_F(RespawnManagerTest, PlayerOneRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();
	}

	EXPECT_EQ(0u, respawnActual);

}

// and for player two
TEST_F(RespawnManagerTest, PlayerTwoRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();
	}

	EXPECT_EQ(0u, respawnActual);

}

// dying again past that leaves the count at nothing rather than wrapping around
TEST_F(RespawnManagerTest, EnemyRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{21u};
	unsigned short respawnActual{21u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		if (!_allObjects.empty())
		{
			_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0u, respawnActual);

}

// the same for the first seat
TEST_F(RespawnManagerTest, PlayerOneRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{4u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		if (!_allObjects.empty())
		{
			_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0u, respawnActual);

}

// and for the second
TEST_F(RespawnManagerTest, PlayerTwoRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{4u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		if (!_allObjects.empty())
		{
			_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0u, respawnActual);

}

// 2P free-for-all is won by the last player standing, not by clearing the bots
TEST_F(RespawnManagerTest, TwoPlayersFreeForAllIsWonByTheLastPlayerStanding)
{
	std::optional<GameState> finished{};
	const EventSubscription finishSub{_events->AddListener([&finished](const GameFinishedEvent& event)
	{
		finished = event.state;
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayersFreeForAll, _respawnManager,
							 _tankSpawner);
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

	ASSERT_FALSE(finished.has_value()) << "the bots were cleared with both players standing, and that ended it";

	for (int life{}; life < 3; ++life)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		killEvery(isPlayerOne);
	}

	EXPECT_EQ(finished, GameState::Won);
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
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
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
	std::optional<Uuid> seatTwo{};
	unsigned short livesLeft{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_events->AddListener([&seatTwo](const RespawnTankEvent& event)
	{
		if (event.type == TankType::PLAYER2)
		{
			seatTwo = event.uuid;
		}
	}));
	subs.push_back(_events->AddListener([&livesLeft](const RespawnCountChangedToEvent& event)
	{
		if (event.group == GroupOf(PlayerSlot::P2))
		{
			livesLeft = event.respawnCount;
		}
	}));
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Bot, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});

	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		seatTwo.reset();
		_events->EmitEvent(RespawnTanksEvent{});
		ASSERT_TRUE(seatTwo.has_value()) << "the bot's seat asked for no tank with a life left";
		_events->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = *seatTwo});
	}
	seatTwo.reset();
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_FALSE(seatTwo.has_value()) << "the control failed - a seat with no lives left asked for a tank";

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Bot});
	EXPECT_EQ(livesLeft, 3u);

	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_TRUE(seatTwo.has_value()) << "the newcomer was given no tank";
	EXPECT_EQ(livesLeft, 2u) << "coming in spent a life the seat never got back";
}

// a client counts a bot's spawn off the seat it sits in, the way the host does
TEST_F(RespawnManagerTest, AClientCountsABotsSpawnOffItsSeat)
{
	unsigned short livesLeft{};
	const EventSubscription countSub{_events->AddListener([&livesLeft](const RespawnCountChangedToEvent& event)
	{
		if (event.group == GroupOf(PlayerSlot::P2))
		{
			livesLeft = event.respawnCount;
		}
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	ASSERT_EQ(livesLeft, 3u);

	_events->EmitEvent(TankRespawnedEvent{.type = TankType::COOP2,
										  .model = TankModel::Player,
										  .uuid = UuidUtils::GetRandomUuid(),
										  .pos = {}});

	EXPECT_EQ(livesLeft, 2u);
}

//NOTE: taken off, not killed - the life of the tank on the field goes back to the seat for the one who comes back
TEST_F(RespawnManagerTest, ASeatGivenUpGetsTheLifeOfItsTankBack)
{
	unsigned short livesLeft{};
	const EventSubscription countSub{_events->AddListener([&livesLeft](const RespawnCountChangedToEvent& event)
	{
		if (event.group == GroupOf(PlayerSlot::P2))
		{
			livesLeft = event.respawnCount;
		}
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_EQ(livesLeft, 2u);

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	EXPECT_EQ(livesLeft, 3u);
}

//NOTE: given up between its tanks - the seat owed a respawn asks for none until its player is back, then gets one
TEST_F(RespawnManagerTest, ASeatGivenUpSpawnsNothingUntilItsPlayerIsBack)
{
	std::vector<RespawnTankEvent> asked{};
	const EventSubscription askedSub{_events->AddListener([&asked](const RespawnTankEvent& event)
	{
		asked.push_back(event);
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	const auto seatTwo{std::ranges::find(asked, TankType::PLAYER2, &RespawnTankEvent::type)};
	ASSERT_NE(seatTwo, asked.end());
	_events->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = seatTwo->uuid});
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});
	asked.clear();

	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_TRUE(asked.empty()) << "the seat given up asked for a tank";

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Empty});
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_EQ(asked.size(), 1u);
	EXPECT_EQ(asked.front().type, TankType::PLAYER2);
}

//NOTE: the one who left was the last on the field - taking that tank off leaves nobody to play the match
TEST_F(RespawnManagerTest, TakingOffTheLastTankOnTheFieldLosesTheMatch)
{
	std::optional<Uuid> seatOne{};
	std::optional<GameState> finished{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_events->AddListener([&seatOne](const RespawnTankEvent& event)
	{
		if (event.type == TankType::PLAYER1)
		{
			seatOne = event.uuid;
		}
	}));
	subs.push_back(_events->AddListener([&finished](const GameFinishedEvent& event) { finished = event.state; }));
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		seatOne.reset();
		_events->EmitEvent(RespawnTanksEvent{});
		ASSERT_TRUE(seatOne.has_value());
		_events->EmitEvent(TankDiedEvent{.who = Author::Player1, .uuid = *seatOne});
	}
	ASSERT_FALSE(finished.has_value()) << "the control failed - the match was over with a tank still on the field";

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	EXPECT_EQ(finished, GameState::Over);
}

// a free-for-all whose enemies are gone is won by the one left alone on the field
TEST_F(RespawnManagerTest, InAFreeForAllTheOneLeftAloneWins)
{
	std::optional<GameState> finished{};
	const EventSubscription finishedSub{_events->AddListener([&finished](const GameFinishedEvent& event)
	{
		finished = event.state;
	})};
	_gameConfig.networkRules = MatchRules::FreeForAll;
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(EnemyLineupLoadedEvent{.count = 0u});
	_events->EmitEvent(RespawnTanksEvent{});
	ASSERT_FALSE(finished.has_value());

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	EXPECT_EQ(finished, GameState::Won);
}

//NOTE: the starting lives come back only for a bot's losses - a player who left with none comes back with none
TEST_F(RespawnManagerTest, APlayerWhoLeftWithNoLivesComesBackWithNone)
{
	std::optional<Uuid> seatTwo{};
	const EventSubscription askedSub{_events->AddListener([&seatTwo](const RespawnTankEvent& event)
	{
		if (event.type == TankType::PLAYER2)
		{
			seatTwo = event.uuid;
		}
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		seatTwo.reset();
		_events->EmitEvent(RespawnTanksEvent{});
		ASSERT_TRUE(seatTwo.has_value());
		_events->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = *seatTwo});
	}
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Empty});
	seatTwo.reset();

	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_FALSE(seatTwo.has_value()) << "coming back gave a seat with no lives left a tank";
}

//NOTE: the tank taken off is no spawn the match still waits to see die - the last death of the rest loses it
TEST_F(RespawnManagerTest, AfterASeatIsGivenUpTheLastDeathOfTheRestLosesTheMatch)
{
	std::optional<Uuid> seatOne{};
	std::optional<GameState> finished{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_events->AddListener([&seatOne](const RespawnTankEvent& event)
	{
		if (event.type == TankType::PLAYER1)
		{
			seatOne = event.uuid;
		}
	}));
	subs.push_back(_events->AddListener([&finished](const GameFinishedEvent& event) { finished = event.state; }));
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	for ([[maybe_unused]] const int life: std::views::iota(0, 3))
	{
		ASSERT_TRUE(seatOne.has_value());
		_events->EmitEvent(TankDiedEvent{.who = Author::Player1, .uuid = *seatOne});
		seatOne.reset();
		_events->EmitEvent(RespawnTanksEvent{});
	}

	EXPECT_EQ(finished, GameState::Over);
}
