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
	float _tankSize{};
	unsigned short _bulletHealth{1u};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = _gameConfig.gridOffset;
		_tankSize = gridSize * 3.f;// for better turns

		CreateBullet({.x = 0.f, .y = 5.f}, Direction::DOWN, 1u, "Bullet1", Faction::PlayerTeam, "Player1");
	}

	void TearDown() override
	{
	}

	//TODO: use this style for others bullet creation
	void CreateBullet(const FPoint pos, const Direction dir, const unsigned short tier, std::string name,
					  Faction faction, std::string author)
	{
		const BulletCalibre calibre{.speed = 300.f,
									.damage = 1u,
									.damageRadius = 12.f,
									.tier = tier,
									.size{.x = 6.f, .y = 5.f}};
		// spawn Bullet
		const ObjRectangle rectBullet{.x = pos.x, .y = pos.y, .w = calibre.size.x, .h = calibre.size.y};
		std::shared_ptr<Bullet> bullet =
				TestUtils::CreateBullet(
						rectBullet, _bulletHealth, _uuid, std::move(name), faction, &_allObjects,
						_events, calibre, dir, _gameMode, _gameConfig, std::move(author));
		_allObjects.emplace_back(bullet);
	}
};

TEST_F(StatisticsTestAdvanced, BulletHitByEnemyBullet)
{
	CreateBullet({.x = 0.f, .y = 5.f + 1}, Direction::UP, 1u, "Bullet2", Faction::EnemyTeam, "Enemy1");

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0u);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1u);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1u);
}

TEST_F(StatisticsTestAdvanced, BulletHitByPlayerOne)
{
	CreateBullet({.x = 0.f, .y = 5.f + 1}, Direction::UP, 1u, "Bullet2", Faction::PlayerTeam, "Player2");

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0u);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1u);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1u);
}
