#pragma once

#include <utility>

#include "../headers/PlayerOne.h"

class MockPlayerOne final : public PlayerOne
{
public:
	MockPlayerOne(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, size_t windowWidth,
				  size_t windowHeight, std::vector<std::shared_ptr<BaseObj>>* allPawns,
				  std::shared_ptr<EventSystem> events)
		: PlayerOne({rect.x, rect.y, rect.w, rect.h}, color, speed, health, windowBuffer, windowWidth, windowHeight,
					allPawns, std::move(events))
	{
	}

	~MockPlayerOne() override = default;

	// MOCK_METHOD(FPoint&, GetPos, (), (const));
	// MOCK_METHOD(void, GetPos, (FPoint&), ());
	// MOCK_METHOD(void, TakeDamage, (int), (override));
	// MOCK_METHOD(void, Move, (float), (override));
	// MOCK_METHOD(std::list<std::weak_ptr<BaseObj>>, IsCanMove, (float), (override));
	// MOCK_METHOD(void, TickUpdate, (float), (override));
	// MOCK_METHOD(void, Shot, (), (override));
	// MOCK_METHOD(void, Draw, (), (const override));

	void MockResetKeyPressed() { keyboardButtons = {}; }
	void MockShotKeyPressed() { keyboardButtons.shot = true; }
	void MockMoveKeyPressed(Direction dir)
	{
		if (dir == LEFT)
		{
			keyboardButtons.a = true;
		}
		else if (dir == RIGHT)
		{
			keyboardButtons.d = true;
		}
		else if (dir == DOWN)
		{
			keyboardButtons.s = true;
		}
		else if (dir == UP)
		{
			keyboardButtons.w = true;
		}
	}
};
