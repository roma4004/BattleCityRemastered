#include "Point.h"
#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>
#include <boost/uuid/random_generator.hpp>

class BulletTestAdvanced : public testing::Test // NOLINT(clang-diagnostic-padded)
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.f / 60.f};
	BulletCalibre _calibre{.speed = 300.f, .damage = 1u, .damageRadius = 12.0, .tier = 3u, .size{.x = 6.f, .y = 5.f}};
	buuid _uuid{};
	float _gridSize{1};
	unsigned short _bulletHealth{1};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;

		_allObjects.reserve(4);

		const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
		std::shared_ptr<Bullet> bullet =
				TestUtils::CreateBullet(
						rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
						_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
		_allObjects.emplace_back(bullet);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(BulletTestAdvanced, BulletTier2CanDestroySteelWall)
{
	if (const Bullet* bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		// spawn BrickWall
		ObjRectangle wallRect = {.x = 0.f, .y = _calibre.size.y + 1, .w = _gridSize, .h = _gridSize};
		auto steelWall = std::make_shared<SteelWall>(wallRect, _events, _uuid, _gameMode);
		_allObjects.emplace_back(steelWall);

		steelWall->SetHealth(1);
		EXPECT_EQ(steelWall->GetHealth(), 1);
		EXPECT_EQ(bullet->GetTier(), 3u);

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(steelWall->GetHealth(), 0);

		return;
	}

	EXPECT_FALSE(true);
}
