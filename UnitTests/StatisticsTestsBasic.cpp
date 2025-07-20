#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/input/InputProviderForPlayerTwo.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class StatisticsTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	int _tankHealth{1};
	int _yellow{0xffffff};
	int _green{0x408000};
	int _gray{0x808080};
	int _bulletHealth{1};
	int _bulletColor{0xffffff};
	int _bulletDamage{1};
	float _tankSize{0.f};
	float _tankSpeed{142.f};
	float _bulletSpeed{300.f};
	float _deltaTimeOneFrame{1.f / 60.f};
	float _bulletWidth{6.f};
	float _bulletHeight{5.f};
	double _bulletDamageRadius{12.0};
	std::string _name{"Player1"};
	std::string _fraction{"PlayerTeam"};
	std::string _name2{"Player2"};
	// std::string _fraction2{"PlayerTeam"};
	std::string _name3{"Enemy1"};
	std::string _fraction3{"EnemyTeam"};
	GameMode _gameMode{GameMode::OnePlayer};
	buuid _uuid{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _gameMode);
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3.f;// for better turns

		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(_events);

		ObjRectangle rect1{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{rect1, _yellow, _tankHealth, _uuid, _name, _fraction};
		PawnProperty pawnProperty{
				std::move(baseObjProperty), &_allObjects, _events, 1, _tankSpeed, _windowSize, Direction::UP,
				_gameMode};

		ObjRectangle rect2{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty2{rect2, _green, _tankHealth, _uuid, _name2, _fraction};
		PawnProperty pawnProperty2{
				std::move(baseObjProperty2), &_allObjects, _events, 1, _tankSpeed, _windowSize, Direction::UP,
				_gameMode};

		ObjRectangle rect3{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty3{rect3, _gray, _tankHealth, _uuid, _name3, _fraction3};
		PawnProperty pawnProperty3{
				std::move(baseObjProperty3), &_allObjects, _events, 1, _tankSpeed, _windowSize, Direction::DOWN,
				_gameMode};

		constexpr bool enableByDefault{true};

		_allObjects.reserve(5);
		BonusEffectProperty bonusEffects{};
		_allObjects.emplace_back(
				std::make_shared<Player>(
						std::move(pawnProperty), _bulletPool, std::move(inputProvider), bonusEffects, enableByDefault));
		_allObjects.emplace_back(
				std::make_shared<Player>(
						std::move(pawnProperty2), _bulletPool, std::move(inputProvider2), bonusEffects,
						enableByDefault));
		_allObjects.emplace_back(
				std::make_shared<Enemy>(std::move(pawnProperty3), _bulletPool, bonusEffects, enableByDefault));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(StatisticsTest, PlayerOneHitByEnemy)
{
	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	ObjRectangle rect{.x = _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerOneHitByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneHitByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerOneHitByFriend)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	ObjRectangle rect{.x = _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author),
			                         enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerOneHitFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneHitFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerTwoHitByEnemy)
{
	//TODO: remove this after moving test to separate file
	_allObjects.pop_back();
	auto backup = _allObjects.back();
	_allObjects.clear();
	_allObjects.emplace_back(backup);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	ObjRectangle rect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerTwoHitByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoHitByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerTwoHitByFriend)
{
	_allObjects.pop_back();
	auto backup = _allObjects.back();
	_allObjects.clear();
	_allObjects.emplace_back(backup);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerTwoHitFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoHitFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerOneDiedByFriend)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerOneDiedByFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerTwoDiedByEnemy)
{
	//TODO: remove this after moving test to separate file
	_allObjects.pop_back();
	auto backup = _allObjects.back();
	_allObjects.clear();
	_allObjects.emplace_back(backup);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	ObjRectangle rect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerOneDiedByEnemy)
{
	//TODO: remove this after moving test to separate file
	_allObjects.pop_back();
	_allObjects.pop_back();

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	ObjRectangle rect{.x = _bulletWidth, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerTwoDiedByFriend)
{
	//TODO: remove this after moving test to separate file
	_allObjects.pop_back();
	auto backup = _allObjects.back();
	_allObjects.clear();
	_allObjects.emplace_back(backup);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerTwoDiedByFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyHitByFriend)
{
	//TODO: remove this after moving test to separate file
	auto backup = _allObjects.back();
	_allObjects.clear();
	_allObjects.emplace_back(backup);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy2"};
	ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                  .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyHitByFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyHitByPlayerOne)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                  .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerOne(), 1);
}

TEST_F(StatisticsTest, EnemyHitByPlayerTwo)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize + 1, .w = _bulletWidth,
	                  .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByFriend)
{
	//TODO: remove this after moving test to separate file
	auto backup = _allObjects.back();
	_allObjects.clear();
	_allObjects.emplace_back(backup);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy2"};
	ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                  .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyDiedByFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerOne)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize + 1, .w = _bulletWidth,
	                  .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerTwo)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                  .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitByPlayerTwo)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"PlayerTeam"};
	std::string author2{"Player2"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty2{
			rect2, _bulletColor, _bulletHealth, _uuid, std::move(name2), std::move(fraction2)};
	PawnProperty pawnProperty2{
			std::move(baseObjProperty2), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty2), _bulletDamage, _bulletDamageRadius, std::move(author2), enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BrickWallDiedByEnemy)
{
	ObjRectangle brickWallRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};

	_allObjects.emplace_back(std::make_shared<BrickWall>(brickWallRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetBrickWallDiedByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, BrickWallDiedByPlayerOne)
{
	ObjRectangle brickWallRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};

	_allObjects.emplace_back(std::make_shared<BrickWall>(brickWallRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, BrickDiedByPlayerTwo)
{
	ObjRectangle brickRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};

	_allObjects.emplace_back(std::make_shared<BrickWall>(brickRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, SteelWallDiedByEnemy)
{
	ObjRectangle brickWallRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};

	_allObjects.emplace_back(std::make_shared<SteelWall>(brickWallRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 3, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetSteelWallDiedByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, SteelWallDiedByPlayerOne)
{
	ObjRectangle brickWallRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};

	_allObjects.emplace_back(std::make_shared<SteelWall>(brickWallRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 3, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, SteelDiedByPlayerTwo)
{
	ObjRectangle brickRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};

	_allObjects.emplace_back(std::make_shared<SteelWall>(brickRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 3, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByEnemy)
{
	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"EnemyTeam"};
	std::string author2{"Enemy2"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty2{
			rect2, _bulletColor, _bulletHealth, _uuid, std::move(name2), std::move(fraction2)};
	PawnProperty pawnProperty2{
			std::move(baseObjProperty2), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty2), _bulletDamage, _bulletDamageRadius, std::move(author2), enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 2);
}

TEST_F(StatisticsTest, BulletHitBulletPlayerOneAndByPlayerTwo)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"PlayerTeam"};
	std::string author2{"Player2"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty2{
			rect2, _bulletColor, _bulletHealth, _uuid, std::move(name2), std::move(fraction2)};
	PawnProperty pawnProperty2{
			std::move(baseObjProperty2), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty2), _bulletDamage, _bulletDamageRadius, std::move(author2), enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerOne)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"EnemyTeam"};
	std::string author2{"Enemy1"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty2{
			rect2, _bulletColor, _bulletHealth, _uuid, std::move(name2), std::move(fraction2)};
	PawnProperty pawnProperty2{
			std::move(baseObjProperty2), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty2), _bulletDamage, _bulletDamageRadius, std::move(author2), enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerTwo)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty{
			rect, _bulletColor, _bulletHealth, _uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::DOWN,
			_gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author), enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"EnemyTeam"};
	std::string author2{"Enemy1"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth, .h = _bulletHeight};
	BaseObjProperty baseObjProperty2{
			rect2, _bulletColor, _bulletHealth, _uuid, std::move(name2), std::move(fraction2)};
	PawnProperty pawnProperty2{
			std::move(baseObjProperty2), &_allObjects, _events, 1, _bulletSpeed, _windowSize, Direction::UP, _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
				std::move(pawnProperty2), _bulletDamage, _bulletDamageRadius, std::move(author2), enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}
