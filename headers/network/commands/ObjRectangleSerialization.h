#pragma once

#include "geometry/ObjRectangle.h"
#include <ser20/ser20.hpp>

namespace ser20
{
//NOTE: as PointSerialization - the type stays a plain struct; only commands that put it on the
//wire pull this in.
template<class Archive>
void serialize(Archive& ar, ObjRectangle& rect, const unsigned int /*version*/)
{
	ar & rect.x;
	ar & rect.y;
	ar & rect.w;
	ar & rect.h;
}
}// namespace ser20
