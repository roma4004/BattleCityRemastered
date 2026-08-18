#pragma once

#include "utils/Uuid.h"
#include <ser20/ser20.hpp>

namespace ser20
{
//NOTE: Uuid lives outside ser20, so the overload goes here where ser20's own lookup finds it.
//16 raw bytes - the same wire format boost::uuids::uuid had.
template<class Archive>
void serialize(Archive& ar, Uuid& uuid, const unsigned int /*version*/)
{
	ar & binary_data(&uuid, sizeof(uuid));
}
}// namespace ser20
