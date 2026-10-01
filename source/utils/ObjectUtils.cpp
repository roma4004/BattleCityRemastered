#include "utils/ObjectUtils.h"
#include "entities/BaseObj.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/IFortress.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "enums/Faction.h"
#include "interfaces/IPickupableBonus.h"

bool ObjectUtils::IsAlive(const std::shared_ptr<BaseObj>& object)
{
	return object != nullptr && object->GetIsAlive();
}

bool ObjectUtils::IsOpponent(const BaseObj& self, const std::shared_ptr<BaseObj>& other)
{
	return other->GetFaction() != self.GetFaction() && other->GetFaction() != Faction::Neutral;
}

bool ObjectUtils::IsAlly(const BaseObj& self, const std::shared_ptr<BaseObj>& other)
{
	return other->GetFaction() == self.GetFaction();
}

bool ObjectUtils::IsBonus(const std::shared_ptr<BaseObj>& object)
{
	return dynamic_cast<IPickupableBonus*>(object.get()) != nullptr;
}

const Bullet* ObjectUtils::AsBullet(const std::shared_ptr<BaseObj>& object)
{
	return dynamic_cast<Bullet*>(object.get());
}

bool ObjectUtils::IsFortress(const std::shared_ptr<BaseObj>& object)
{
	return dynamic_cast<IFortress*>(object.get()) != nullptr;
}

bool ObjectUtils::IsWall(const std::shared_ptr<BaseObj>& object)
{
	return dynamic_cast<BrickWall*>(object.get()) != nullptr || dynamic_cast<SteelWall*>(object.get()) != nullptr;
}
