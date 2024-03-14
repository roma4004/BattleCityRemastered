#pragma once
#include "SDL.h"
#include <list>

#include "../headers/BaseObj.h"
#include "../headers/Environment.h"
#include "../headers/IDrawable.h"
#include "../headers/IMovable.h"
#include "../headers/PlayerKeys.h"
#include "../headers/Point.h"

struct Point;
struct Environment;

enum Direction { UP = 0, LEFT = 1, DOWN = 2, RIGHT = 3 };

class Pawn : public BaseObj, public IMovable
{
public:
	Pawn(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);

	~Pawn() override;

	void MarkDestroy(Environment* env) const override;

	void Move(Environment* env) override;

	void Draw(const Environment* env) const override;

	virtual void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key);

	[[nodiscard]] static bool IsCollideWith(const SDL_Rect* rect1, const SDL_Rect* rect2);

	[[nodiscard]] virtual std::tuple<bool, std::list<BaseObj*>> IsCanMove(const BaseObj* me, const Environment* env);

	void TickUpdate(Environment* env) override;

	virtual void Shot(Environment* env);

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