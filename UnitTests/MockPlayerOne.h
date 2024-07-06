#pragma once

#include "../headers/PlayerOne.h"

class MockPlayerOne final : public PlayerOne
{
public:
	MockPlayerOne(const Rectangle& rect, const int color, const int health, int* windowBuffer, const UPoint windowSize,
	              Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	              std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
	              std::unique_ptr<IInputProvider>& inputProvider, std::shared_ptr<BulletPool> bulletPool)
		: PlayerOne({rect.x, rect.y, rect.w, rect.h}, color, health, windowBuffer, windowSize, direction, speed,
		            allObjects, std::move(events), std::move(name), std::move(fraction), inputProvider,
		            std::move(bulletPool)) {}

	~MockPlayerOne() override = default;

	// MOCK_METHOD(FPoint&, GetPos, (), (const));
	// MOCK_METHOD(void, GetPos, (FPoint&), ());
	// MOCK_METHOD(void, TakeDamage, (int), (override));
	// MOCK_METHOD(void, Move, (float), (override));
	// MOCK_METHOD(std::list<std::weak_ptr<BaseObj>>, IsCanMove, (float), (override));
	// MOCK_METHOD(void, TickUpdate, (float), (override));
	// MOCK_METHOD(void, Shot, (), (override));
	// MOCK_METHOD(void, Draw, (), (const override));
};
