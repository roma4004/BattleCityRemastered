#pragma once

#include "BaseObjProperty.h"
#include "TextureManager.h"
#include "ObjRectangle.h"
#include "interfaces/IDrawable.h"
#include "interfaces/IHaveFraction.h"
#include "interfaces/IObstacle.h"
#include "interfaces/ISendableDamageStatistics.h"

struct FPoint;

class BaseObj : public IObstacle, public IDrawable, public ISendableDamageStatistics, public IHaveFraction
{
	int _color{0};
	int _health{0};
	bool _isAlive{true};

protected:
	boost::uuids::uuid _uuid{};
	std::string _name;
	std::string _nameWithUuid;
	std::string _fraction;
	ObjRectangle _rect{};

public:
	explicit BaseObj(BaseObjProperty baseObjProperty);

	BaseObj(ObjRectangle rect, int color, int health, boost::uuids::uuid uuid, std::string name, std::string fraction);

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

	[[nodiscard]] virtual ObjRectangle GetRect() const;
	virtual void SetRect(ObjRectangle rect);

	[[nodiscard]] virtual std::string GetName() const;
	[[nodiscard]] virtual boost::uuids::uuid GetUuid() const;
	virtual void SetId(boost::uuids::uuid uuid);
	[[nodiscard]] std::string GetFraction() const override;
};
