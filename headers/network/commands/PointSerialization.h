#pragma once

#include "geometry/Point.h"
#include <ser20/ser20.hpp>

namespace ser20
{
//NOTE: non-intrusive, so Point.h stays free of ser20 - only the command serialization includes this
template<class Archive>
void serialize(Archive& ar, FPoint& point, const unsigned int /*version*/)
{
	ar & point.x;
	ar & point.y;
}

template<class Archive>
void serialize(Archive& ar, Point& point, const unsigned int /*version*/)
{
	ar & point.x;
	ar & point.y;
}

template<class Archive>
void serialize(Archive& ar, UPoint& point, const unsigned int /*version*/)
{
	ar & point.x;
	ar & point.y;
}
}// namespace ser20
