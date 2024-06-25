#pragma once

#include "../headers/PlayerOne.h"

#include <utility>

class MockPlayerOne final : public PlayerOne
{
public:
	MockPlayerOne(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
	              const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
	              std::shared_ptr<EventSystem> events)
		: PlayerOne({rect.x, rect.y, rect.w, rect.h}, color, speed, health, windowBuffer, windowSize, allPawns,
		            std::move(events)) {}

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

	void MockMoveKeyPressed(const Direction dir)
	{
		if (dir == LEFT)
		{
			keyboardButtons.left = true;
		}
		else if (dir == RIGHT)
		{
			keyboardButtons.right = true;
		}
		else if (dir == DOWN)
		{
			keyboardButtons.down = true;
		}
		else if (dir == UP)
		{
			keyboardButtons.up = true;
		}
	}

	void SetDirection(Direction direction) const { _moveBeh->SetDirection(direction); }
};