#include "utils/ObjectUtils.h"
#include "entities/BaseObj.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/IFortress.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "enums/Faction.h"
#include "interfaces/IPickupableBonus.h"

bool ObjectUtils::IsAlive(const BaseObj* const object)
{
	return object != nullptr && object->GetIsAlive();
}

bool ObjectUtils::IsOpponent(const BaseObj& self, const BaseObj& other)
{
	const Faction faction{other.GetFaction()};

	return faction != Faction::Neutral && (faction != self.GetFaction() || faction == Faction::Solo);
}

bool ObjectUtils::IsAlly(const BaseObj& self, const BaseObj& other)
{
	return other.GetFaction() == self.GetFaction() && self.GetFaction() != Faction::Solo;
}

bool ObjectUtils::IsBonus(const BaseObj& object)
{
	return dynamic_cast<const IPickupableBonus*>(&object) != nullptr;
}

const Bullet* ObjectUtils::AsBullet(const BaseObj& object)
{
	return dynamic_cast<const Bullet*>(&object);
}

bool ObjectUtils::IsShellOf(const BaseObj& object, const Uuid& shooter)
{
	const Bullet* const bullet{AsBullet(object)};

	return bullet != nullptr && shooter != Uuid{} && bullet->GetAuthorUuid() == shooter;
}

bool ObjectUtils::IsFortress(const BaseObj& object)
{
	return dynamic_cast<const IFortress*>(&object) != nullptr;
}

bool ObjectUtils::IsWall(const BaseObj& object)
{
	return dynamic_cast<const BrickWall*>(&object) != nullptr || dynamic_cast<const SteelWall*>(&object) != nullptr;
}
