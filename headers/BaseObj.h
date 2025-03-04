#pragma once

#include "ObjRectangle.h"
#include "interfaces/IDrawable.h"
#include "interfaces/IObstacle.h"
#include "interfaces/ISendableDamageStatistics.h"

struct FPoint;

class BaseObj : public IObstacle, public IDrawable, public ISendableDamageStatistics
{
	int _color{0};
	int _health{0};
	bool _isAlive{true};

protected:
	int _id{0};
	std::string _name;
	ObjRectangle _shape{};

public:
	BaseObj(const ObjRectangle& rect, int color, int health, int id, std::string name);

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

	[[nodiscard]] virtual int GetHealth() const;
	virtual void SetHealth(int health);

	[[nodiscard]] virtual bool GetIsAlive() const;
	virtual void SetIsAlive(bool isAlive);

	virtual void TakeDamage(int damage);

	[[nodiscard]] bool GetIsPassable() const override;
	void SetIsPassable(bool value) override;

	[[nodiscard]] bool GetIsDestructible() const override;
	void SetIsDestructible(bool value) override;

	[[nodiscard]] bool GetIsPenetrable() const override;
	void SetIsPenetrable(bool value) override;

	[[nodiscard]] virtual ObjRectangle GetShape() const;
	virtual void SetShape(ObjRectangle shape);
};
