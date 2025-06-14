#include "../headers/components/EventSystem.h"
#include "../headers/components/GameStatistics.h"
#include "../headers/enums/Direction.h"
#include "../headers/enums/GameMode.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/input/InputProviderForPlayerTwo.h"
#include "../headers/pawns/Bullet.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PawnProperty.h"
#include "../headers/pawns/Player.h"
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
	int _bulletColor{0xffffff};
	int _bulletDamage{1};
	float _tankSize{0.f};
	float _tankSpeed{142.f};
	float _bulletSpeed{300.f};
	float _deltaTimeOneFrame{1.f / 60.f};
	float _bulletWidth{6.f};
	float _bulletHeight{5.f};
	double _bulletDamageRadius{12.0};
	GameMode _gameMode{OnePlayer};
	buuid _uuid{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3.f;// for better turns

		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(_events);

		const std::string name{"Bullet1"};
		const std::string fraction{"PlayerTeam"};
		const std::string author{"Player1"};
		ObjRectangle rect{.x = 0.f, .y = _bulletHeight, .w = _bulletWidth, .h = _bulletHeight};
		CreateBullet(name, fraction, author, 0.f, _bulletHeight, DOWN);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}

	void CreateBullet(std::string name, std::string fraction, std::string author, float x, float y, Direction dir)
	{
		ObjRectangle rect2{.x = x, .y = y, .w = _bulletWidth, .h = _bulletHeight};
		BaseObjProperty baseObjProperty2{
				rect2, _bulletColor, _bulletHealth, true, _uuid, std::move(name), std::move(fraction)};
		PawnProperty pawnProperty2{
				std::move(baseObjProperty2), &_allObjects, _events, _windowSize, _gameMode, 1, UP, _bulletSpeed};

		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						std::move(pawnProperty2), _bulletDamage, _bulletDamageRadius, std::move(author)));
	}
};

TEST_F(StatisticsTestAdvanced, BulletHitByEnemyBullet)
{
	const std::string name{"Bullet2"};
	const std::string fraction{"EnemyTeam"};
	const std::string author{"Enemy1"};
	CreateBullet(name, fraction, author, 0.f, _bulletHeight + 1, UP);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
}

TEST_F(StatisticsTestAdvanced, BulletHitByPlayerOne)
{
	const std::string name{"Bullet2"};
	const std::string fraction{"PlayerTeam"};
	const std::string author{"Player2"};
	CreateBullet(name, fraction, author, 0.f, _bulletHeight + 1, UP);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}
