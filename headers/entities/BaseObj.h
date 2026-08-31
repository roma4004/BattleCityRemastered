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

	//NOTE: an entity is its uuid - copying one would hand two objects the same identity
	BaseObj(const BaseObj&) = delete;
	BaseObj(BaseObj&&) = delete;
	BaseObj& operator=(const BaseObj&) = delete;
	BaseObj& operator=(BaseObj&&) = delete;

	~BaseObj() override;

public:
	//NOTE: entering and leaving the world, not construction and destruction - an object is
	//built first and subscribes only when the world takes it, so the call reaches the leaf
	virtual void Activate() {}
	virtual void Deactivate() {}

	[[nodiscard]] FPoint GetPos() const;
	void SetPos(const FPoint& pos);
	[[nodiscard]] double GetRightSide() const;

	[[nodiscard]] double GetBottomSide() const;
	[[nodiscard]] double GetX() const;
	void SetX(const FPoint& pos);

	[[nodiscard]] double GetY() const;
	void SetY(const FPoint& pos);

	[[nodiscard]] double GetWidth() const;
	void SetWidth(double width);

	[[nodiscard]] double GetHeight() const;
	void SetHeight(double height);

	void MoveX(double i);
	void MoveY(double i);

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
