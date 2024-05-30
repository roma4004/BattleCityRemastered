#pragma once

#include "Rectangle.h"

#include "IDrawable.h"
#include "IObstacle.h"
#include "ITickUpdatable.h"

struct FPoint;

class BaseObj : public IObstacle, public IDrawable, public ITickUpdatable
{
public:
	BaseObj(const Rectangle& rect, int color, int health);

	~BaseObj() override;

	[[nodiscard]] FPoint GetPos() const;
	void SetPos(const FPoint& pos);
	[[nodiscard]] float GetRightSide() const;

	[[nodiscard]] float GetBottomSide() const;
	[[nodiscard]] float GetX() const;
	void SetX(const FPoint& pos);

	[[nodiscard]] float GetY() const;
	void SetY(const FPoint& pos);

	[[nodiscard]] float GetWidth() const;
	void SetWidth(float width);

	[[nodiscard]] float GetHeight() const;
	void SetHeight(float height);

	void MoveX(float i);
	void MoveY(float i);

	[[nodiscard]] int GetColor() const;
	void SetColor(int color);

	[[nodiscard]] int GetHealth() const;
	void SetHealth(int health);

	[[nodiscard]] bool GetIsAlive() const;
	void SetIsAlive(bool isAlive);

	virtual void TakeDamage(int damage);

	[[nodiscard]] bool GetIsPassable() const override;
	void SetIsPassable(bool value) override;

	[[nodiscard]] bool GetIsDestructible() const override;
	void SetIsDestructible(bool value) override;

	[[nodiscard]] bool GetIsPenetrable() const override;
	void SetIsPenetrable(bool value) override;

	[[nodiscard]] const Rectangle& GetShape() const;

protected:
	Rectangle shape{};

private:
	int _color{0};
	int _health{0};
	bool _isAlive{true};
};
