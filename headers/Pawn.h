#pragma once
#include "SDL.h"

#include <functional>
#include <list>
#include <memory>

#include "../headers/BaseObj.h"
#include "../headers/Environment.h"
#include "../headers/IMovable.h"
#include "../headers/PlayerKeys.h"
#include "../headers/Point.h"

struct FPoint;
struct Environment;

enum Direction
{
	UP = 0,
	LEFT = 1,
	DOWN = 2,
	RIGHT = 3
};

class Pawn : public BaseObj, public IMovable
{
public:
	Pawn(const FPoint& pos, float width, float height, int color, float speed, int health, Environment* env);

	~Pawn() override;

	float FindNearestDistance(const std::list<std::weak_ptr<BaseObj>>& pawns,
							  const std::function<float(const std::shared_ptr<BaseObj>&)>& getNearestSide) const;

	void Move() override;

	void Draw() const override;

	virtual void KeyboardEvensHandlers(Uint32 eventType, SDL_Keycode key);

	[[nodiscard]] static bool IsCollideWith(const Rectangle& r1, const Rectangle& r2);

	[[nodiscard]] virtual std::tuple<bool, std::list<std::weak_ptr<BaseObj>>> IsCanMove(const BaseObj* me);

	void TickUpdate() override;

	virtual void Shot();

	[[nodiscard]] Direction GetDirection() const;

	void SetDirection(Direction direction);

	[[nodiscard]] int GetBulletWidth() const;
	void SetBulletWidth(int bulletWidth);

	[[nodiscard]] int GetBulletHeight() const;
	void SetBulletHeight(int bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	PlayerKeys keyboardButtons;

private:
	Direction _direction = Direction::UP;

	int _damage{1};

	int _bulletWidth{6};

	int _bulletHeight{6};

	float _bulletSpeed{300};
};
