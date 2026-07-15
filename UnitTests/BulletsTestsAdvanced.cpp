#include "Point.h"
#include "components/EventSystem.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>
#include <boost/uuid/random_generator.hpp>

class BulletTestAdvanced : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800u, .y = 600u};
	int _bulletHealth{1};
	float _gridSize{1};
	double _deltaTimeOneFrame{1.f / 60.f};
	BulletCalibre _calibre{.speed = 300.f, .damage = 1, .damageRadius = 12.0, .tier = 3u, .size{.x = 6.f, .y = 5.f}};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;

		std::string name{"Bullet1"};
		std::string fraction{"PlayerTeam"};
		std::string author{"Player1"};
		const ObjRectangle rect{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
		BaseObjProperty baseObjProperty{
				.rect = rect,
				.health = _bulletHealth,
				.uuid = _uuid,
				.name = std::move(name),
				.fraction = std::move(fraction)};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = _calibre.tier,
				.speed = _calibre.speed,
				.windowSize = _windowSize,
				.dir = Direction::DOWN,
				.gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<Bullet>(std::move(pawnProperty), _calibre, std::move(author), enableByDefault));
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
		ObjRectangle wallRect = {.x = 0.f, .y = _calibre.size.y + 1, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<SteelWall>(wallRect, _events, _uuid, _gameMode));

		if (SteelWall* steelWall = dynamic_cast<SteelWall*>(_allObjects.back().get()))
		{
			steelWall->SetHealth(1);
			EXPECT_EQ(steelWall->GetHealth(), 1);
			EXPECT_EQ(bullet->GetTier(), 3u);

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(steelWall->GetHealth(), 0);
		}
		else
		{
			EXPECT_FALSE(true);
		}
	}
	else
	{
		EXPECT_FALSE(true);
	}
}
