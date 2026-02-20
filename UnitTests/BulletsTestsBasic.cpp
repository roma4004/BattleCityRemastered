#include "Point.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class BulletTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	int _bulletDamage{1};
	int _bulletHealth{1};
	unsigned int _bulletColor{0xffffff}; 
	FPoint _bulletSize;
	float _bulletSpeed{300.f};
	float _gridSize{1};
	float _tankSize{};
	float _tankSpeed{142.f};
	float _bulletWidth{6.f};
	float _bulletHeight{5.f};
	double _deltaTimeOneFrame{1.f / 60.f};
	double _bulletDamageRadius{12.0};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_bulletSize = FPoint{.x = 6.f, .y = 5.f};

		std::string name{"Bullet1"};
		std::string fraction{"PlayerTeam"};
		std::string author{"Player1"};
		const ObjRectangle rect{.x = 0.f, .y = 0.f, .w = _bulletSize.x, .h = _bulletSize.y};

		BaseObjProperty baseObjProperty{
				.rect = rect, .color = _bulletColor, .health = _bulletHealth, .uuid = _uuid, .name = std::move(name),
				.fraction = std::move(fraction)};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty), .allObjects = &_allObjects, .events = _events, .tier = 1,
				.speed = _bulletSpeed, .windowSize = _windowSize, .dir = Direction::DOWN, .gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author),
						enableByDefault));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

//TODO: extract to base object tests like set pos
// Check that tank set their position correctly
TEST_F(BulletTest, BulletSetPos)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({});
		const FPoint startPos = bullet->GetPos();

		bullet->SetPos({.x = static_cast<float>(_windowSize.x), .y = static_cast<float>(_windowSize.y)});

		EXPECT_LT(startPos, bullet->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that a bullet can move inside the screen
TEST_F(BulletTest, BulletMoveInsideScreen)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		{
			//success bullet moves down test, try to move inside a screen bullet
			bullet->SetDirection(Direction::DOWN);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_LT(bulletStartPos.y, bulletEndPos.y);
			EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
		}
		{
			//success bullet right test, try to move inside a screen bullet
			bullet->SetDirection(Direction::RIGHT);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_LT(bulletStartPos.x, bulletEndPos.x);
			EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
		}

		const auto windowWidth = static_cast<float>(_windowSize.x);
		const auto windowHeight = static_cast<float>(_windowSize.y);

		bullet->SetPos({.x = windowWidth - _bulletSize.x, .y = windowHeight - _bulletSize.y});
		{
			//success shot up test, try to create an inside screen bullet
			bullet->SetDirection(Direction::UP);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_GT(bulletStartPos.y, bulletEndPos.y);
			EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
		}
		{
			//success move left test, try to move inside a screen bullet
			bullet->SetDirection(Direction::LEFT);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_GT(bulletStartPos.x, bulletEndPos.x);
			EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that a bullet can't move outside the screen
TEST_F(BulletTest, BulletMoveOutSideScreen)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		const auto windowWidth = static_cast<float>(_windowSize.x);
		const auto windowHeight = static_cast<float>(_windowSize.y);

		bullet->SetPos({.x = windowWidth - _bulletSize.x, .y = windowHeight - _bulletSize.y});
		{
			//fail bullet move down test, try to move an outside screen bullet
			bullet->SetDirection(Direction::DOWN);
			bullet->SetPos({.x = windowWidth - _bulletSize.x, .y = windowHeight - _bulletSize.y});
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}
		{
			//fail the bullet move right test, try to move an outside screen bullet
			bullet->SetDirection(Direction::RIGHT);
			bullet->SetPos({.x = windowWidth - _bulletSize.x, .y = windowHeight - _bulletSize.y});
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}

		bullet->SetPos({.x = 0.f, .y = 0.f});
		{
			//fail bullet move up test, try to move an outside screen bullet
			bullet->SetDirection(Direction::UP);
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}
		{
			//fail the bullet move left test, try to move an outside screen bullet
			bullet->SetDirection(Direction::LEFT);
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that a bullet can deal damage to another obstacle and self
TEST_F(BulletTest, BulletDamageBrickWhenMoveUp)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 7.f});
		bullet->SetDirection(Direction::UP);
		ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));
		if (const auto brickWall = dynamic_cast<const BrickWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickWallHealth = brickWall->GetHealth();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickWallHealth, brickWall->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that a bullet can deal damage to another obstacle and self
TEST_F(BulletTest, BulletDamageBrickWhenMoveLeft)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 7.f, .y = 0.f});
		bullet->SetDirection(Direction::LEFT);
		ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));
		if (const auto brickWall = dynamic_cast<const BrickWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickWallHealth = brickWall->GetHealth();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickWallHealth, brickWall->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that a bullet can deal damage to another obstacle and self
TEST_F(BulletTest, BulletDamageBrickWhenMoveDown)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(Direction::DOWN);
		ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));
		if (const auto brick = dynamic_cast<const BrickWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that a bullet can deal damage to another obstacle and self
TEST_F(BulletTest, BulletDamageBrickWhenMoveRight)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(Direction::RIGHT);
		ObjRectangle rect{.x = 7.f, .y = 0.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));
		if (const auto brick = dynamic_cast<const BrickWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(BulletTest, BulletDamageTank)
{
	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	const float tankSize = gridSize * 3;// for better turns
	constexpr int tankHealth = 1;
	constexpr int gray = 0x808080;
	auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _gameMode);

	ObjRectangle rect{.x = 0, .y = _bulletSize.y, .w = tankSize, .h = tankSize};
	BaseObjProperty baseObjProperty{.rect = rect, .color = gray, .health = tankHealth, .uuid = _uuid, .name = "Enemy1",
	                                .fraction = "EnemyTeam"};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty), .allObjects = &_allObjects, .events = _events, .tier = 1,
			.speed = _bulletSpeed, .windowSize = _windowSize, .dir = Direction::UP, .gameMode = _gameMode};

	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Enemy>(
					std::move(pawnProperty), std::move(bulletPool), BonusEffectProperty{}, enableByDefault));

	const auto enemy = dynamic_cast<const Enemy*>(_allObjects.back().get());

	EXPECT_EQ(enemy->GetHealth(), 1);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(enemy->GetHealth(), 0);
}

// Check that a bullet can deal damage to another bullet and self
TEST_F(BulletTest, BulletToBulletDamageEachOther)
{
	if (const auto bullet = dynamic_cast<const Bullet*>(_allObjects.front().get()))
	{
		std::string name{"Bullet2"};
		std::string fraction{"PlayerTeam"};
		std::string author{"Player2"};
		ObjRectangle rect{.x = 0, .y = _bulletSize.y + 1, .w = _bulletSize.x, .h = _bulletSize.y};
		BaseObjProperty baseObjProperty{
				.rect = rect, .color = _bulletColor, .health = _bulletHealth, .uuid = _uuid, .name = std::move(name),
				.fraction = std::move(fraction)};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty), .allObjects = &_allObjects, .events = _events, .tier = 1,
				.speed = _bulletSpeed, .windowSize = _windowSize, .dir = Direction::UP, .gameMode = _gameMode};

		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author)));

		if (const auto bullet2 = dynamic_cast<const Bullet*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int bullet2Health = bullet2->GetHealth();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(bullet2Health, bullet2->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that a bullet can't deal damage to an undestractable object
TEST_F(BulletTest, BulletCantDamageSteelWall)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(Direction::DOWN);
		ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameMode));
		if (const auto brick = dynamic_cast<const SteelWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_EQ(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that a bullet can pass through water
TEST_F(BulletTest, BulletCantDamageWater)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(Direction::DOWN);
		ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameMode));
		if (const auto brick = dynamic_cast<const WaterTile*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletHealth, bullet->GetHealth());
			EXPECT_EQ(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(BulletTest, BulletDamagefortressWall)
{
	if (auto&& bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(Direction::DOWN);
		ObjRectangle rect{.x = 0.f, .y = 6.f, .w = 36, .h = 36};
		_allObjects.emplace_back(
				std::make_shared<FortressWall>(rect, _events, &_allObjects, _uuid, _gameMode));
		if (const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get()))
		{
			fortressWall->SetHealth(1);
			EXPECT_EQ(fortressWall->GetHealth(), 1);

			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(fortressWall->GetHealth(), -1);

			return;
		}
	}

	EXPECT_TRUE(false);
}
