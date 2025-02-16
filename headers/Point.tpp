#pragma once

#include "Point.h"

template<class Archive>
void FPoint::serialize(Archive& ar, const unsigned int /*version*/) {
	ar & x;
	ar & y;
}

template<class Archive>
void Point::serialize(Archive& ar, const unsigned int /*version*/) {
	ar & x;
	ar & y;
}

template<class Archive>
void UPoint::serialize(Archive& ar, const unsigned int /*version*/) {
	ar & x;
	ar & y;
}
