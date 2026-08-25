#pragma once

#include "geometry/ObjRectangle.h"
#include "entities/Tags.h"
#include "enums/Faction.h"
#include "interfaces/IObstacle.h"
#include "utils/Uuid.h"
#include <string>

struct FPoint;
struct BaseObjProperty;

class BaseObj : public IObstacle
{
	int _health{0};
	CollisionTags _collision;

protected:
	Uuid _uuid{};
	std::string _name{};
	std::string _nameWithUuid{};
	Faction _faction{};
	ObjRectangle _rect{};

	virtual void EmitDamageStatistics(const std::string& author, Faction faction);
	virtual void EmitDeathStatistics(const std::string& author, Faction faction);

	BaseObj(BaseObjProperty baseObjProperty, CollisionTags collision);

	BaseObj(const BaseObj& other);
	BaseObj(BaseObj&& other) noexcept;

	BaseObj& operator=(const BaseObj& other);
	BaseObj& operator=(BaseObj&& other) noexcept;

public:
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

	[[nodiscard]] virtual int GetHealth() const;
	virtual void SetHealth(int health);

	[[nodiscard]] bool GetIsAlive() const override;
	void SetIsAlive(bool isAlive) override;

	virtual void TakeDamage(unsigned int damage, const std::string& /*author*/, Faction /*faction*/);

	[[nodiscard]] bool GetIsPassable() const override;

	[[nodiscard]] bool GetIsDestructible() const override;

	[[nodiscard]] bool GetIsPenetrable() const override;

	[[nodiscard]] virtual ObjRectangle GetRect() const;
	virtual void SetRect(ObjRectangle rect);

	[[nodiscard]] virtual std::string GetName() const;
	[[nodiscard]] virtual Uuid GetUuid() const;
	virtual void SetId(Uuid uuid);
	[[nodiscard]] Faction GetFaction() const;
};
