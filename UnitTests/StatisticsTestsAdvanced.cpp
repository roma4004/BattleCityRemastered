#include "components/EventSystem.h"
#include "components/GameStatistics.h"
// #include "components/input/InputProviderForPlayerOne.h"
// #include "components/input/InputProviderForPlayerTwo.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Enemy.h"
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
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	int _bulletHealth{1};
	unsigned int _bulletColor{0xffffff};
	int _bulletDamage{1};
	float _tankSize{};
	// float _tankSpeed{142.f};
	float _bulletSpeed{300.f};
	float _bulletWidth{6.f};
	float _bulletHeight{5.f};
	double _deltaTimeOneFrame{1.f / 60.f};
	double _bulletDamageRadius{12.0};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3.f;// for better turns

		// std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);
		// std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(_events);

		const std::string name{"Bullet1"};
		const std::string fraction{"PlayerTeam"};
		const std::string author{"Player1"};
		// ObjRectangle rect{.x = 0.f, .y = _bulletHeight, .w = _bulletWidth, .h = _bulletHeight};
		CreateBullet(name, fraction, author, 0.f, _bulletHeight, Direction::DOWN);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}

	void CreateBullet(std::string name, std::string fraction, std::string author, float x, float y, Direction dir)
	{
		ObjRectangle rect2{.x = x, .y = y, .w = _bulletWidth, .h = _bulletHeight};
		BaseObjProperty baseObjProperty2{
				.rect = rect2, .color = _bulletColor, .health = _bulletHealth, .uuid = _uuid, .name = std::move(name),
				.fraction = std::move(fraction)};
		PawnProperty pawnProperty2{
				.baseObjProperty = std::move(baseObjProperty2), .allObjects = &_allObjects, .events = _events, .tier = 1,
				.speed = _bulletSpeed, .windowSize = _windowSize, .dir = dir, .gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						std::move(pawnProperty2), _bulletDamage, _bulletDamageRadius, std::move(author),
						enableByDefault));
	}
};

TEST_F(StatisticsTestAdvanced, BulletHitByEnemyBullet)
{
	const std::string name{"Bullet2"};
	const std::string fraction{"EnemyTeam"};
	const std::string author{"Enemy1"};
	CreateBullet(name, fraction, author, 0.f, _bulletHeight + 1, Direction::UP);

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
	CreateBullet(name, fraction, author, 0.f, _bulletHeight + 1, Direction::UP);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}
