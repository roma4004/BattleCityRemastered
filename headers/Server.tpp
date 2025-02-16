#pragma once

#include "Server.h"

template<class Archive>
void Data::serialize(Archive& ar, const unsigned int) {
	ar & health;
	ar & objectName;
	ar & eventName;
	ar & names;
	ar & newPos;
	ar & direction;
}
