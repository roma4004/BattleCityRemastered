#include "entities/pawns/Bullet.h"
#include "utils/Log.h"
#include "application/GameConfig.h"
#include "behavior/MoveLikeBulletBeh.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/Sprite.h"
#include "components/WorldGeometry.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Author.h"
#include "enums/GameMode.h"
#include "enums/Terrain.h"
#include "enums/TextureType.h"
#include "interfaces/IMoveBeh.h"
#include "network/commands/WorldSnapshot.h"
#include "utils/DirectionUtils.h"
#include "utils/ObjectUtils.h"
#include "utils/UuidUtils.h"
#include <algorithm>
#include <optional>
#include <ranges>
#include <vector>

Bullet::Bullet(PawnProperty pawnProperty, const GameConfig& gameConfig, const BulletCaliber& caliber)
	: Pawn{std::move(pawnProperty), gameConfig, kCollision}
	, _caliber{caliber}
{
	auto moveBeh{std::make_unique<MoveLikeBulletBeh>(_rect, _uuid, _authorUuid, _gameConfig, _caliber)};
	_bulletMoveBeh = moveBeh.get();
	_moveBeh = std::move(moveBeh);
}

Bullet::~Bullet()
{
	Log::Detail("bullet destroyed " + UuidUtils::GetStringUuid(_uuid));
}

void Bullet::Subscribe()
{
	Pawn::Subscribe();

	if (_gameConfig.IsHost())
	{
		_subs.push_back(_events->AddListener(this, &Bullet::OnWorldSnapshotRequested));
	}
}

void Bullet::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	event.snapshot.bullets.push_back(
			network::commands::BulletSnapshot{.author = _author, .uuid = _uuid, .rect = _rect, .dir = _dir});
}

void Bullet::OnDespawned(const DespawnedEvent& event)
{
	Pawn::OnDespawned(event);

	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect});
}

std::optional<Sprite> Bullet::Look() const
{
	return Sprite{.rect = _rect, .layer = kLayer, .dir = _dir, .texture = TextureType::Bullet};
}

Uuid Bullet::GetUuid() const
{
	return _uuid;
}

Uuid Bullet::GetAuthorUuid() const noexcept { return _authorUuid; }

void Bullet::Reset(const BulletResetProperty& resetProperty)
{
	_rect = resetProperty.rect;
	_dir = resetProperty.dir;

	_author = resetProperty.author;
	_authorUuid = resetProperty.authorUuid;
	_faction = FactionOf(_author, _gameConfig.Rules());
	_caliber = resetProperty.caliber;
	SetHealth(_caliber.health);
	_depthInWall = 0.0;

	_isAlive = true;
}

void Bullet::TickUpdate(const double deltaTime)
{
	std::vector<BaseObj*> blast;
	if (_moveBeh->Move(_dir, deltaTime, _allObjects, blast))
	{
		if (_gameConfig.IsHost())
		{
			_events->EmitEvent(PosChangedEvent{.pos = GetPos(), .dir = _dir, .uuid = _uuid});
		}

		return;
	}

	const std::vector<BaseObj*> contacts{_bulletMoveBeh->GetContacts(_dir, deltaTime, _allObjects)};
	const auto isShell = [](const BaseObj* const contact) { return dynamic_cast<const Bullet*>(contact) != nullptr; };
	//NOTE: shells in the way - they trade health, and any left at zero deals its damage
	if (std::ranges::any_of(contacts, isShell))
	{
		MeetShells(contacts, blast);

		return;
	}

	//NOTE: a breakable wall the shell has health and depth for breaks at its touch, and the shell flies on
	if (PenetrateWall(contacts))
	{
		return;
	}

	//NOTE: anything else - a tank, a bonus, the eagle, steel, a wall too tough or too deep, the field edge
	DealDamage(blast);
}

//NOTE: each shell loses the other's health as it was before the hit; a survivor flies on, one at zero deals damage
void Bullet::MeetShells(const std::vector<BaseObj*>& contacts, const std::vector<BaseObj*>& blast)
{
	const unsigned int ownHealth{GetHealth()};
	for (BaseObj* const contact: contacts)
	{
		auto* const shell{dynamic_cast<Bullet*>(contact)};
		if (shell == nullptr)
		{
			continue;
		}

		const unsigned int shellHealth{shell->GetHealth()};
		shell->TakeDamage(ownHealth, _author);
		TakeDamage(shellHealth, shell->GetAuthor());
		//NOTE: a meeting counts as a hit for both shooters
		_events->EmitEvent(StatisticsBulletHitEvent{.author = shell->GetAuthor()});
		if (!shell->GetIsAlive())
		{
			shell->DealDamage(shell->_bulletMoveBeh->GetBlastWhereItStands(_allObjects));
		}
	}

	_events->EmitEvent(StatisticsBulletHitEvent{.author = _author});
	if (!GetIsAlive())
	{
		DealDamage(blast);
	}
}

double Bullet::GetFlightSpeed() const noexcept { return _caliber.speed; }

//NOTE: steel gives way from the third tier on
bool Bullet::CanBreak(const BaseObj& target) const noexcept { return target.GetIsDestructible() || _caliber.tier > 2u; }

//NOTE: a layer costs its toughest brick's health and breaks only within the depth BlastDepthFor allows
bool Bullet::PenetrateWall(const std::vector<BaseObj*>& contacts)
{
	const bool isWall{!contacts.empty() && std::ranges::all_of(contacts, [this](const BaseObj* const contact)
	{
		return ObjectUtils::IsWall(*contact) && CanBreak(*contact);
	})};
	if (!isWall)
	{
		return false;
	}

	const unsigned int cost{std::ranges::max(contacts | std::views::transform(&BaseObj::GetHealth))};
	if (GetHealth() <= cost)
	{
		return false;
	}

	//NOTE: GetContacts keeps only the nearest ones - all at one gap, so any of them is the layer
	const ObjRectangle layer{contacts.front()->GetRect()};
	//NOTE: a gap behind the last broken layer starts a new wall
	if (DirectionUtils::GapTo(_wallBack, layer, _dir) > 0.0)
	{
		_depthInWall = 0.0;
	}

	const double layerDepth{DirectionUtils::SizeAlong(layer, _dir)};
	const double centerDepth{_depthInWall + layerDepth - DirectionUtils::SizeAlong(_rect, _dir) / 2.0};
	if (centerDepth > WorldGeometry::BlastDepthFor(_gameConfig.tankSize, _caliber.blastRadius))
	{
		return false;
	}

	TakeDamage(cost, Author::None);
	_wallBack = layer;
	_depthInWall += layerDepth;
	for (BaseObj* const contact: contacts)
	{
		contact->TakeDamage(_caliber.damage, _author);
	}

	return true;
}

//NOTE: the blast skips shells (they trade health instead), water and ice; steel and bushes need CanBreak's tier
void Bullet::DealDamage(const std::vector<BaseObj*>& blast)
{
	const auto isHit = [this](const BaseObj* const target)
	{
		const Terrain terrain{target->GetTerrain()};

		return terrain != Terrain::Water && terrain != Terrain::Ice && dynamic_cast<const Bullet*>(target) == nullptr
			   && CanBreak(*target);
	};

	std::ranges::for_each(blast | std::views::filter(isHit), [this](BaseObj* const target)
	{
		target->TakeDamage(_caliber.damage, _author);
	});
	//NOTE: BaseObj's TakeDamage, not Pawn's - a shell dealing its damage was not shot, so no HealthChangedEvent
	BaseObj::TakeDamage(GetHealth(), _author);

	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect});
}
