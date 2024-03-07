#pragma once

#include "../headers/IDestroyable.h"
#include "../headers/IDrawable.h"
#include "../headers/IObsticle.h"
#include "../headers/ITickUpdatable.h"
#include "../headers/Point.h"

struct Environment;

class BaseObj : public IObsticle, public IDrawable, public ITickUpdatable, public IDestroyable
{
public:
	BaseObj(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);

	~BaseObj() override;

	[[nodiscard]] float GetX() const;
	void SetX(const Point& pos);

	[[nodiscard]] float GetY() const;
	void SetY(const Point& pos);

	[[nodiscard]] int GetWidth() const;
	void SetWidth(int width);

	[[nodiscard]] int GetHeight() const;
	void SetHeight(int height);

	void MoveX(float i);
	void MoveY(float i);

	[[nodiscard]] float GetSpeed() const;

	[[nodiscard]] int GetColor() const;
	void SetColor(int color);

	[[nodiscard]] int GetHealth() const;
	void SetHealth(int health);

	[[nodiscard]] bool GetIsAlive() const;
	void SetIsAlive(bool isAlive);

	virtual void TakeDamage(int damage);

	[[nodiscard]] bool GetIsPassable() override;
	void SetIsPassable(bool value) override;

	[[nodiscard]] bool GetIsDestructible() override;
	void SetIsDestructible(bool value) override;

	[[nodiscard]] bool GetIsPenetrable() override;
	void SetIsPenetrable(bool value) override;

private:
	float _x{};
	float _y{};
	int _width{};
	int _height{};
	int _color{};
	float _speed{};
	int _health{};
	bool _isAlive = true;

protected:
	Environment* _env = nullptr;
};