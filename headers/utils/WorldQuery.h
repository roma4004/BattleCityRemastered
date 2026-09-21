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

	//NOTE: ground a tank drives over - bush and ice - leaves the spot free; water and everything solid
	//do not, and neither does a bullet passing through
	[[nodiscard]] static bool IsSpotFreeOfBlockers(const std::vector<std::shared_ptr<BaseObj>>& objects,
												   const ObjRectangle& rect);

	//NOTE: only a pawn blocks a rebuild - the spot always overlaps the wall standing in it
	[[nodiscard]] static bool IsSpotFreeOfPawns(const std::vector<std::shared_ptr<BaseObj>>& objects,
												const ObjRectangle& rect);
};
