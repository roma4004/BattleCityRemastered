#pragma once

#include "Pawn.h"
#include "entities/BulletCalibre.h"
#include "interfaces/IDrawable.h"
#include "utils/Uuid.h"
#include <memory>
#include <vector>

enum class Faction : char8_t;
struct BulletResetProperty;
struct UPoint;
struct DrawEvent;
struct DespawnedEvent;
class EventSystem;
class BulletPool;
class GameConfig;

class Bullet final : public Pawn, public IDrawable
{
	friend BulletPool;

	Uuid _authorUuid{};
	BulletCalibre _calibre{};

	void Reset(const BulletResetProperty& resetProperty);
	void OnDraw(const DrawEvent&) const;

protected:
	void Subscribe() override;
	void EmitDamageStatistics(Author author) override;
	void OnDespawned(const DespawnedEvent& event) override;
	void Draw() const override;
	void TickUpdate(double deltaTime) override;

public:
	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Destructible{}, tags::Impenetrable{}};

	Bullet(PawnProperty pawnProperty, const GameConfig& gameConfig, const BulletCalibre& calibre = {});

	~Bullet() override;

	[[nodiscard]] unsigned int GetDamage() const;

	[[nodiscard]] double GetDamageRadius() const;

	[[nodiscard]] Uuid GetUuid() const override;

	[[nodiscard]] unsigned int GetTier() const;

	void DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList);
};
