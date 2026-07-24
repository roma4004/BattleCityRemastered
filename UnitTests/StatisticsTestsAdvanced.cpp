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
	BulletCalibre _calibre{.speed = 300.f, .damage = 1, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.f, .y = 5.f}};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = gridSize * 3.f;// for better turns

		const std::string name{"Bullet1"};
		const std::string fraction{"PlayerTeam"};
		const std::string author{"Player1"};
		// const ObjRectangle rect{.x = 0.f, .y = _bulletHeight, .w = _bulletWidth, .h = _bulletHeight};
		CreateBullet(name, fraction, author, 0.f, _calibre.size.y, Direction::DOWN);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}

	void CreateBullet(std::string name, std::string fraction, std::string author, float x, float y, Direction dir)
	{
		ObjRectangle rect2{.x = x, .y = y, .w = _calibre.size.x, .h = _calibre.size.y};
		BaseObjProperty baseObjProperty2{
				.rect = rect2,
				.health = _bulletHealth,
				.uuid = _uuid,
				.name = std::move(name),
				.fraction = std::move(fraction)};
		PawnProperty pawnProperty2{
				.baseObjProperty = std::move(baseObjProperty2),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1u,
				.speed = _calibre.speed,
				.dir = dir,
				.gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.emplace_back(
				std::make_shared<Bullet>(std::move(pawnProperty2), _gameConfig, _calibre, std::move(author),
										 enableByDefault));
	}
};

TEST_F(StatisticsTestAdvanced, BulletHitByEnemyBullet)
{
	const std::string name{"Bullet2"};
	const std::string fraction{"EnemyTeam"};
	const std::string author{"Enemy1"};
	CreateBullet(name, fraction, author, 0.f, _calibre.size.y + 1, Direction::UP);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
}

TEST_F(StatisticsTestAdvanced, BulletHitByPlayerOne)
{
	const std::string name{"Bullet2"};
	const std::string fraction{"PlayerTeam"};
	const std::string author{"Player2"};
	CreateBullet(name, fraction, author, 0.f, _calibre.size.y + 1, Direction::UP);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}
