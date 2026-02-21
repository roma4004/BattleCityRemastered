#pragma once

#include "entities/ObjRectangle.h"
#include "interfaces/IHaveFraction.h"
#include "interfaces/IObstacle.h"
#include "interfaces/ISendableDamageStatistics.h"
#include <boost/uuid/uuid.hpp>

struct FPoint;
struct BaseObjProperty;

class BaseObj : public ISendableDamageStatistics, public IHaveFraction, public IObstacle
{
	using buuid = boost::uuids::uuid;

	int _health{0};

protected:
	buuid _uuid{};
	unsigned int _color{0};
	std::string _name{};
	std::string _nameWithUuid{};
	std::string _fraction{};
	ObjRectangle _rect{};

public:
	explicit BaseObj(BaseObjProperty baseObjProperty);

	BaseObj(const BaseObj& other);
	BaseObj(BaseObj&& other) noexcept;

	BaseObj(ObjRectangle rect, unsigned int color, int health, buuid uuid, std::string name, std::string fraction);

	~BaseObj() override;

	BaseObj& operator=(const BaseObj& other);
	BaseObj& operator=(BaseObj&& other) noexcept;

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

	[[nodiscard]] unsigned int GetColor() const;
	void SetColor(unsigned int color);

	[[nodiscard]] virtual int GetHealth() const;
	virtual void SetHealth(int health);

	[[nodiscard]] bool GetIsAlive() const override;
	void SetIsAlive(bool isAlive) override;

	virtual void TakeDamage(int damage);

	[[nodiscard]] bool GetIsPassable() const override;
	void SetIsPassable(bool value) override;

	[[nodiscard]] bool GetIsDestructible() const override;
	void SetIsDestructible(bool value) override;

	[[nodiscard]] bool GetIsPenetrable() const override;
	void SetIsPenetrable(bool value) override;

	[[nodiscard]] virtual ObjRectangle GetRect() const;
	virtual void SetRect(ObjRectangle rect);

	[[nodiscard]] virtual std::string_view GetName() const;
	[[nodiscard]] virtual buuid GetUuid() const;
	virtual void SetId(buuid uuid);
	[[nodiscard]] std::string GetFraction() const override;
};
