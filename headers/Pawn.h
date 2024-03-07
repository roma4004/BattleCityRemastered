#pragma once
#include "SDL.h"

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

	[[nodiscard]] bool IsCollideWith(const SDL_Rect* self, const Pawn* other) const;

	[[nodiscard]] virtual std::tuple<bool, Pawn*> IsCanMove(const SDL_Rect* self, const Environment* env) const;

	void TickUpdate(Environment* env) override;

	virtual void Shot(Environment* env);

	[[nodiscard]] Direction GetDirection() const;

	void SetDirection(Direction direction);

	[[nodiscard]] int GetBulletWidth() const;
	void SetBulletWidth(int bulletWidth);

	[[nodiscard]] int GetBulletHeight() const;
	void SetBulletHeight(int bulletHeight);

	[[nodiscard]] int GetBulletSpeed() const;
	void SetBulletSpeed(int bulletSpeed);

	PlayerKeys keyboardButtons;

private:
	Direction _direction = Direction::UP;

	int _damage{1};

	int _bulletWidth{10};

	int _bulletHeight{10};

	int _bulletSpeed{5};
};