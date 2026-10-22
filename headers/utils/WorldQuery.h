#pragma once

#include <memory>
#include <vector>

struct ObjRectangle;
class BaseObj;

class WorldQuery final
{
public:
	[[nodiscard]] static bool IsSpotFree(const std::vector<std::shared_ptr<BaseObj>>& objects,
										 const ObjRectangle& rect);

	//NOTE: bush and ice leave the spot free; water, solid objects and bullets do not
	[[nodiscard]] static bool IsSpotFreeOfBlockers(const std::vector<std::shared_ptr<BaseObj>>& objects,
												   const ObjRectangle& rect);

	//NOTE: only a pawn blocks a rebuild - the spot always overlaps the wall standing in it
	[[nodiscard]] static bool IsSpotFreeOfPawns(const std::vector<std::shared_ptr<BaseObj>>& objects,
												const ObjRectangle& rect);
};
