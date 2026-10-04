#pragma once

#include "Pawn.h"
#include "components/Sprite.h"
#include "entities/BulletCaliber.h"
#include "enums/DrawLayer.h"
#include "utils/Uuid.h"
#include <memory>
#include <optional>
#include <vector>

enum class Faction : char8_t;
struct BulletResetProperty;
struct UPoint;
struct DespawnedEvent;
struct WorldSnapshotRequestedEvent;
class EventSystem;
class MoveLikeBulletBeh;
class BulletPool;
class GameConfig;

class Bullet final : public Pawn
{
	friend BulletPool;

	Uuid _authorUuid{};
	BulletCaliber _caliber{};
	MoveLikeBulletBeh* _bulletMoveBeh{nullptr};

	void Reset(const BulletResetProperty& resetProperty);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	[[nodiscard]] bool CanBreak(const BaseObj& target) const noexcept;
	[[nodiscard]] bool SinkIntoWall(double deltaTime, const std::vector<BaseObj*>& blast);
	//NOTE: whether it met another shell - one that did is spent by the meeting, not burnt out by itself
	[[nodiscard]] bool Blast(const std::vector<BaseObj*>& objectList);

protected:
	void Subscribe() override;
	void OnDespawned(const DespawnedEvent& event) override;
	void TickUpdate(double deltaTime) override;

public:
	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Destructible{}, tags::Impenetrable{},
											  tags::NoTerrain{}};
	static constexpr DrawLayer kLayer{DrawLayer::World};

	Bullet(PawnProperty pawnProperty, const GameConfig& gameConfig, const BulletCaliber& caliber = {});

	~Bullet() override;

	[[nodiscard]] unsigned int GetDamage() const noexcept;

	[[nodiscard]] double GetDamageRadius() const noexcept;

	//NOTE: the caliber's speed and not Pawn::GetSpeed - MoveLikeBulletBeh steps the bullet by this one
	[[nodiscard]] double GetFlightSpeed() const noexcept;

	[[nodiscard]] Uuid GetUuid() const override;

	[[nodiscard]] Uuid GetAuthorUuid() const noexcept;

	[[nodiscard]] unsigned int GetTier() const noexcept;

	void DealDamage(const std::vector<BaseObj*>& objectList);

	[[nodiscard]] std::optional<Sprite> Look() const override;
};
