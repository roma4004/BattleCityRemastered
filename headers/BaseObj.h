#pragma once

#include "../headers/IObsticle.h"
#include "../headers/Point.h"

class BaseObj : public IObsticle
{
public:
	BaseObj(const Point& pos, int width, int height, int color, int speed, int health);

	virtual ~BaseObj();

	[[nodiscard]] int GetX() const;
	void SetX(const Point& pos);

	[[nodiscard]] int GetY() const;
	void SetY(const Point& pos);

	[[nodiscard]] int GetWidth() const;
	void SetWidth(int width);

	[[nodiscard]] int GetHeight() const;
	void SetHeight(int height);

	void MoveX(int i);
	void MoveY(int i);

	[[nodiscard]] int GetSpeed() const;

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
	int _x{};
	int _y{};
	int _width{};
	int _height{};
	int _color{};
	int _speed{};
	int _health{};
	bool _isAlive = true;
};
