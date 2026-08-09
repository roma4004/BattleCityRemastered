#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "components/GameStatistics.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class StatisticsTestAdvanced : public testing::Test// NOLINT(clang-diagnostic-padded)
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.f / 60.f};
	buuid _uuid{};
	float _tankSize{};
	unsigned short _bulletHealth{1u};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = gridSize * 3.f;// for better turns

		CreateBullet({.x = 0.f, .y = 5.f}, Direction::DOWN, 1u, "Bullet1", "PlayerTeam", "Player1");
	}

	void TearDown() override
	{
		_events->RemoveListener<AddToSpawnQueueEvent>("TestSpawnQueue");
	}

	//TODO: use this style for others bullet creation
	void CreateBullet(const FPoint pos, const Direction dir, const unsigned short tier, std::string name,
					  std::string fraction, std::string author)
	{
		const BulletCalibre calibre{.speed = 300.f,
									.damage = 1u,
									.damageRadius = 12.0,
									.tier = tier,
									.size{.x = 6.f, .y = 5.f}};
		// spawn Bullet
		const ObjRectangle rectBullet{.x = pos.x, .y = pos.y, .w = calibre.size.x, .h = calibre.size.y};
		std::shared_ptr<Bullet> bullet =
				TestUtils::CreateBullet(
						rectBullet, _bulletHealth, _uuid, std::move(name), std::move(fraction), &_allObjects,
						_events, calibre, dir, _gameMode, _gameConfig, std::move(author));
		_allObjects.emplace_back(bullet);
	}
};

TEST_F(StatisticsTestAdvanced, BulletHitByEnemyBullet)
{
	CreateBullet({.x = 0.f, .y = 5.f + 1}, Direction::UP, 1u, "Bullet2", "EnemyTeam", "Enemy1");

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0u);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1u);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1u);
}

TEST_F(StatisticsTestAdvanced, BulletHitByPlayerOne)
{
	CreateBullet({.x = 0.f, .y = 5.f + 1}, Direction::UP, 1u, "Bullet2", "PlayerTeam", "Player2");

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0u);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1u);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1u);
}
