#include "../headers/Point.h"
#include "../headers/application/Window.h"
#include "../headers/components/EventSystem.h"
#include "../headers/enums/Direction.h"
#include "../headers/enums/GameMode.h"
#include "../headers/obstacles/FortressWall.h"
#include "../headers/obstacles/SteelWall.h"
#include "../headers/pawns/Bullet.h"
#include "../headers/pawns/PawnProperty.h"

#include "gtest/gtest.h"

#include <memory>
#include <boost/uuid/random_generator.hpp>

class BulletTestAdvanced : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<Window> _window{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	FPoint _bulletSize;
	float _bulletSpeed{300.f};
	float _gridSize{1};
	float _deltaTimeOneFrame{1.f / 60.f};
	GameMode _gameMode{OnePlayer};
	int _bulletDamage{1};
	int _bulletHealth{1};
	int _bulletColor{0xffffff};
	float _bulletWidth{6.f};
	float _bulletHeight{5.f};
	double _bulletDamageRadius{12.0};
	buuid _uuid{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_window = std::make_shared<Window>(UPoint{.x = 800, .y = 600}, std::shared_ptr<int[]>());
		_gridSize = static_cast<float>(_window->size.y) / 50.f;
		_bulletSize = FPoint{.x = 6.f, .y = 5.f};

		std::string name{"Bullet1"};
		std::string fraction{"PlayerTeam"};
		std::string author{"Player1"};
		ObjRectangle rect{.x = 0.f, .y = 0.f, .w = _bulletSize.x, .h = _bulletSize.y};
		BaseObjProperty baseObjProperty{
				rect, _bulletColor, _bulletHealth, true, _uuid, std::move(name), std::move(fraction)};
		PawnProperty pawnProperty{
				std::move(baseObjProperty), &_allObjects, _events, _window, nullptr, _gameMode, 3, DOWN, _bulletSpeed};

		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<Bullet>(std::move(pawnProperty), _bulletDamage, _bulletDamageRadius,
				                         std::move(author)));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(BulletTestAdvanced, BulletTier2CanDestroySteelWall)
{
	if (/*auto&& bullet = */dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		ObjRectangle wallRect = {.x = 0.f, .y = _bulletSize.y + 1, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<SteelWall>(wallRect, _window, _events, _uuid, _gameMode, nullptr));

		if (const auto steelWall = dynamic_cast<SteelWall*>(_allObjects.back().get()))
		{
			steelWall->SetHealth(1);
			EXPECT_EQ(steelWall->GetHealth(), 1);

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(steelWall->GetHealth(), 0);
		}
	}
}
