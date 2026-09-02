#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "components/GameStatistics.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
#include <memory>

class StatisticsTestAdvanced : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	unsigned short _bulletHealth{1u};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_statistics = std::make_shared<GameStatistics>(_events);
		const double gridSize = _gameConfig.gridOffset;
		_tankSize = gridSize * 3.0;// for better turns

		CreateBullet({.x = 0.0, .y = 5.0}, Direction::DOWN, 1u, "Bullet1", Faction::PlayerTeam, Author::Player1);
	}

	void TearDown() override {}

	//TODO: use this style for others bullet creation
	void CreateBullet(const FPoint pos, const Direction dir, const unsigned short tier, std::string name,
					  const Faction faction, const Author author)
	{
		const BulletCalibre calibre{.speed = 300.0,
									.damage = 1u,
									.damageRadius = 12.0,
									.tier = tier,
									.size{.x = 6.0, .y = 5.0}};
		// spawn Bullet
		const ObjRectangle rectBullet{.x = pos.x, .y = pos.y, .w = calibre.size.x, .h = calibre.size.y};
		std::shared_ptr<Bullet> bullet =
				TestUtils::CreateBullet(
						rectBullet, _bulletHealth, _uuid, std::move(name), faction, _allObjects,
						_events, calibre, dir, _gameMode, _gameConfig, author);
		_allObjects.emplace_back(bullet);
	}
};

TEST_F(StatisticsTestAdvanced, BulletHitByEnemyBullet)
{
	CreateBullet({.x = 0.0, .y = 5.0 + 1}, Direction::UP, 1u, "Bullet2", Faction::EnemyTeam, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 1u);
}

TEST_F(StatisticsTestAdvanced, BulletHitByPlayerOne)
{
	CreateBullet({.x = 0.0, .y = 5.0 + 1}, Direction::UP, 1u, "Bullet2", Faction::PlayerTeam, Author::Player2);

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 1u);
}
