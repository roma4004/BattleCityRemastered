#pragma once

#include "../headers/PlayerOne.h"

class MockPlayerOne final : public PlayerOne
{
public:
	MockPlayerOne(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
	              const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
	              std::shared_ptr<EventSystem> events, std::string name, std::unique_ptr<IInputProvider>& inputProvider,
	              std::shared_ptr<BulletPool> bulletPool)
		: PlayerOne({rect.x, rect.y, rect.w, rect.h}, color, speed, health, windowBuffer, windowSize, allPawns,
		            std::move(events), std::move(name), inputProvider, std::move(bulletPool)) {}

	~MockPlayerOne() override = default;

	// MOCK_METHOD(FPoint&, GetPos, (), (const));
	// MOCK_METHOD(void, GetPos, (FPoint&), ());
	// MOCK_METHOD(void, TakeDamage, (int), (override));
	// MOCK_METHOD(void, Move, (float), (override));
	// MOCK_METHOD(std::list<std::weak_ptr<BaseObj>>, IsCanMove, (float), (override));
	// MOCK_METHOD(void, TickUpdate, (float), (override));
	// MOCK_METHOD(void, Shot, (), (override));
	// MOCK_METHOD(void, Draw, (), (const override));

	void MockResetKeyPressed() const { _inputProvider->playerKeys = {}; }
	void MockShotKeyPressed() const { _inputProvider->playerKeys.shot = true; }

	void MockMoveKeyPressed(const Direction dir) const
	{
		if (dir == LEFT)
		{
			_inputProvider->playerKeys.left = true;
		}
		else if (dir == RIGHT)
		{
			_inputProvider->playerKeys.right = true;
		}
		else if (dir == DOWN)
		{
			_inputProvider->playerKeys.down = true;
		}
		else if (dir == UP)
		{
			_inputProvider->playerKeys.up = true;
		}
	}

	void SetDirection(Direction direction) const { _moveBeh->SetDirection(direction); }
};
