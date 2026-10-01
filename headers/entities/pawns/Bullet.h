#pragma once

#include "Pawn.h"
#include "entities/BulletCaliber.h"
#include "interfaces/IDrawable.h"
#include "utils/Uuid.h"
#include <memory>
#include <vector>

enum class Faction : char8_t;
struct BulletResetProperty;
struct UPoint;
struct DrawEvent;
struct DespawnedEvent;
struct WorldSnapshotRequestedEvent;
class EventSystem;
class MoveLikeBulletBeh;
class BulletPool;
class GameConfig;

class Bullet final : public Pawn, public IDrawable
{
	friend BulletPool;

	Uuid _authorUuid{};
	BulletCaliber _caliber{};
	MoveLikeBulletBeh* _bulletMoveBeh{nullptr};

	void Reset(const BulletResetProperty& resetProperty);
	void OnDraw(const DrawEvent&) const;
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	[[nodiscard]] bool CanBreak(const BaseObj& target) const noexcept;
	[[nodiscard]] bool SinkIntoWall(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& blast);
	//NOTE: whether it met another shell - one that did is spent by the meeting, not burnt out by itself
	[[nodiscard]] bool Blast(const std::vector<std::shared_ptr<BaseObj>>& objectList);

protected:
	void Subscribe() override;
	void OnDespawned(const DespawnedEvent& event) override;
	void Draw() const override;
	void TickUpdate(double deltaTime) override;

public:
	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Destructible{}, tags::Impenetrable{},
											  tags::NoTerrain{}};

	Bullet(PawnProperty pawnProperty, const GameConfig& gameConfig, const BulletCaliber& caliber = {});

	~Bullet() override;

	[[nodiscard]] unsigned int GetDamage() const noexcept;

	[[nodiscard]] double GetDamageRadius() const noexcept;

	//NOTE: the caliber's speed and not Pawn::GetSpeed - MoveLikeBulletBeh steps the bullet by this one
	[[nodiscard]] double GetFlightSpeed() const noexcept;

	[[nodiscard]] Uuid GetUuid() const override;

	[[nodiscard]] unsigned int GetTier() const noexcept;

	void DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList);
};
