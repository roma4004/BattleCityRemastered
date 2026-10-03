#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/managers/RespawnManager.h"
#include "entities/TankModelSpec.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/GameMode.h"
#include "enums/TankModel.h"
#include "enums/TankType.h"
#include "geometry/Point.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

// a model is rolled per enemy, and its numbers have to reach the tank, the wire and back
class TankModelTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_allObjects.reserve(8u);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager,
								 _tankSpawner);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
	}

	[[nodiscard]] std::vector<std::shared_ptr<Tank>> TanksOnField() const
	{
		std::vector<std::shared_ptr<Tank>> tanks;
		for (const std::shared_ptr<BaseObj>& object: _allObjects)
		{
			if (auto tank{std::dynamic_pointer_cast<Tank>(object)})
			{
				tanks.push_back(std::move(tank));
			}
		}

		return tanks;
	}
};

// health and speed are the base figures scaled by the model the tank was rolled as, whichever one that was
TEST_F(TankModelTest, AnEnemyIsBuiltToTheNumbersOfItsModel)
{
	_events->EmitEvent(RespawnTanksEvent{});

	const std::vector<std::shared_ptr<Tank>> tanks{TanksOnField()};
	ASSERT_FALSE(tanks.empty());

	for (const std::shared_ptr<Tank>& tank: tanks)
	{
		if (tank->GetAuthor() == Author::Player1 || tank->GetAuthor() == Author::Player2)
		{
			continue;
		}

		const TankModel model{tank->GetModel()};
		EXPECT_EQ(tank->GetHealth(), HealthOf(model, _gameConfig.tankHealth)) << ToString(model);
		EXPECT_DOUBLE_EQ(tank->GetSpeed(), SpeedOf(model, _gameConfig.tankSpeed)) << ToString(model);
	}
}

TEST_F(TankModelTest, APlayerSeatCarriesThePlayerModel)
{
	_events->EmitEvent(RespawnTanksEvent{});

	const std::vector<std::shared_ptr<Tank>> tanks{TanksOnField()};
	const auto isPlayer = [](const std::shared_ptr<Tank>& tank) { return tank->GetAuthor() == Author::Player1; };
	const auto player{std::ranges::find_if(tanks, isPlayer)};
	ASSERT_NE(player, tanks.end());

	EXPECT_EQ((*player)->GetModel(), TankModel::Player);
	EXPECT_EQ((*player)->GetHealth(), _gameConfig.tankHealth);
	EXPECT_DOUBLE_EQ((*player)->GetSpeed(), _gameConfig.tankSpeed);
}

TEST_F(TankModelTest, AMirroredSpawnIsBuiltToTheModelTheHostRolled)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	const Uuid uuid{UuidUtils::GetRandomUuid()};

	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1,
										  .model = TankModel::Armor,
										  .uuid = uuid,
										  .pos = FPoint{.x = 0.0, .y = 0.0}});
	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = uuid});

	const std::vector<std::shared_ptr<Tank>> tanks{TanksOnField()};
	ASSERT_EQ(tanks.size(), 1u);
	EXPECT_EQ(tanks.front()->GetModel(), TankModel::Armor);
	EXPECT_EQ(tanks.front()->GetHealth(), HealthOf(TankModel::Armor, _gameConfig.tankHealth));
}

// the map's list decides the models, enemy by enemy in its order
TEST_F(TankModelTest, EnemiesComeAsTheMapListsThem)
{
	const std::vector lineup{TankModel::Armor, TankModel::Fast, TankModel::Power};
	_events->EmitEvent(EnemyLineupLoadedEvent{.count = lineup.size(), .models = lineup});

	std::vector<TankModel> models;
	for (const TankType seat: {TankType::ENEMY1, TankType::ENEMY2, TankType::ENEMY3})
	{
		_events->EmitEvent(RespawnTankEvent{.type = seat, .uuid = UuidUtils::GetRandomUuid()});
		models.push_back(std::dynamic_pointer_cast<Tank>(_allObjects.back())->GetModel());
	}

	EXPECT_EQ(models, lineup);
}

// past the end of the list the model is rolled - an enemy owed beyond it still comes
TEST_F(TankModelTest, AnEnemyPastTheEndOfTheListStillComes)
{
	_events->EmitEvent(EnemyLineupLoadedEvent{.count = 2u, .models = {TankModel::Armor}});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::ENEMY1, .uuid = UuidUtils::GetRandomUuid()});
	const std::shared_ptr<BaseObj> listed{_allObjects.back()};
	_events->EmitEvent(RespawnTankEvent{.type = TankType::ENEMY2, .uuid = UuidUtils::GetRandomUuid()});

	EXPECT_EQ(std::dynamic_pointer_cast<Tank>(listed)->GetModel(), TankModel::Armor);
	EXPECT_NE(_allObjects.back(), listed) << "the enemy past the list never came";
}
