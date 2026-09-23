#include "utils/WorldQuery.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Pawn.h"
#include "geometry/ObjRectangle.h"
#include "utils/ColliderUtils.h"
#include "utils/ObjectUtils.h"
#include "enums/Terrain.h"
#include <algorithm>

namespace
{
//NOTE: passable terrain and nothing else - a bullet is passable too, and spawning on one is a free hit
[[nodiscard]] bool IsDrivableGround(const BaseObj& object)
{
	return object.GetIsPassable() && object.GetTerrain() != Terrain::None;
}
}//namespace

bool WorldQuery::IsSpotFree(const std::vector<std::shared_ptr<BaseObj>>& objects, const ObjRectangle& rect)
{
	return std::ranges::none_of(objects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ObjectUtils::IsAlive(object) && ColliderUtils::IsCollide(rect, object->GetRect());
	});
}

bool WorldQuery::IsSpotFreeOfBlockers(const std::vector<std::shared_ptr<BaseObj>>& objects,
									  const ObjRectangle& rect)
{
	return std::ranges::none_of(objects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ObjectUtils::IsAlive(object) && !IsDrivableGround(*object)
			   && ColliderUtils::IsCollide(rect, object->GetRect());
	});
}

bool WorldQuery::IsSpotFreeOfPawns(const std::vector<std::shared_ptr<BaseObj>>& objects, const ObjRectangle& rect)
{
	return std::ranges::none_of(objects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ObjectUtils::IsAlive(object) && dynamic_cast<const Pawn*>(object.get()) != nullptr
			   && ColliderUtils::IsCollide(rect, object->GetRect());
	});
}
