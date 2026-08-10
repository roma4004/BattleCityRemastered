#pragma once

#include "entities/ObjRectangle.h"
#include "entities/Tags.h"
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
	CollisionTags _collision;

protected:
	buuid _uuid{};
	std::string _name{};
	std::string _nameWithUuid{};
	std::string _fraction{};
	ObjRectangle _rect{};

public:
	BaseObj(BaseObjProperty baseObjProperty, CollisionTags collision);

	BaseObj(const BaseObj& other);
	BaseObj(BaseObj&& other) noexcept;

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

	[[nodiscard]] virtual int GetHealth() const;
	virtual void SetHealth(int health);

	[[nodiscard]] bool GetIsAlive() const override;
	void SetIsAlive(bool isAlive) override;

	virtual void TakeDamage(unsigned int damage, const std::string& /*damageAuthor*/,
							const std::string& /*damageFraction*/);

	[[nodiscard]] bool GetIsPassable() const override;

	[[nodiscard]] bool GetIsDestructible() const override;

	[[nodiscard]] bool GetIsPenetrable() const override;

	[[nodiscard]] virtual ObjRectangle GetRect() const;
	virtual void SetRect(ObjRectangle rect);

	[[nodiscard]] virtual std::string GetName() const;
	[[nodiscard]] virtual buuid GetUuid() const;
	virtual void SetId(buuid uuid);
	[[nodiscard]] std::string GetFraction() const override;
};
