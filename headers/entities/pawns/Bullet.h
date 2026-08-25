#pragma once

#include "Pawn.h"
#include "entities/BulletCalibre.h"
#include "interfaces/IDrawable.h"
#include "utils/Uuid.h"
#include <string>

enum class Faction : char8_t;
struct BulletResetProperty;
struct UPoint;
class EventSystem;
class BulletPool;
class ShootingBeh;
class GameConfig;
struct DrawEvent;
struct DespawnedEvent;
struct PosChangedEvent;

class Bullet final : public Pawn, public IDrawable
{
	friend BulletPool;
	friend ShootingBeh;

	std::string _author{};
	BulletCalibre _calibre{};

	void SubscribeAsClient() override;
	void Enable();
	void Disable();
	void Reset(BulletResetProperty resetProperty);
	void OnDraw(const DrawEvent&) const;

protected:
	void Subscribe() override;
	void EmitDamageStatistics(const std::string& author, Faction faction) override;
	void OnDespawned(const DespawnedEvent& event) override;
	void Draw() const override;
	void TickUpdate(double deltaTime) override;

public:
	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Destructible{}, tags::Impenetrable{}};

	Bullet(PawnProperty pawnProperty, const GameConfig& gameConfig, const BulletCalibre& calibre = {},
		   std::string author = "", bool enableByDefault = false);

	~Bullet() override;

	[[nodiscard]] unsigned int GetDamage() const;

	[[nodiscard]] float GetDamageRadius() const;

	[[nodiscard]] std::string GetAuthor() const;

	[[nodiscard]] Uuid GetUuid() const override;
	[[nodiscard]] const std::string& GetUuidStr() const;

	[[nodiscard]] unsigned int GetTier() const;

	void DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList);
	void OnPosChanged(const PosChangedEvent& event);
};
