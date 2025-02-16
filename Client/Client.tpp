#pragma once

#include "Client.h"

template<class Archive>
void Data::serialize(Archive& ar, const unsigned int version) {
	ar & health;
	ar & objectName;
	ar & eventName;
	ar & names;
	ar & newPos;
	ar & direction;
}
