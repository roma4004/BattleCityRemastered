#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class StatisticsTestAdvanced : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	int _bulletHealth{1};
	float _tankSize{};
	double _deltaTimeOneFrame{1.f / 60.f};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = gridSize * 3.f;// for better turns

		CreateBullet({.x = 0.f, .y = 5.f}, Direction::DOWN, 1u, "Bullet1", "PlayerTeam", "Player1");
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}

	//TODO: use this style for others bullet creation
	void CreateBullet(const FPoint pos, const Direction dir, const unsigned int tier, std::string name,
					  std::string fraction,
					  std::string author)
	{
		const BulletCalibre calibre{.speed = 300.f,
									.damage = 1,
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

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
}

TEST_F(StatisticsTestAdvanced, BulletHitByPlayerOne)
{
	CreateBullet({.x = 0.f, .y = 5.f + 1}, Direction::UP, 1u, "Bullet2", "PlayerTeam", "Player2");

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}
