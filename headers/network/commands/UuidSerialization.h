#pragma once

#include "utils/Uuid.h"
#include <ser20/ser20.hpp>

namespace ser20
{
//NOTE: in namespace ser20, where its lookup finds the overload; 16 raw bytes, the layout boost::uuids::uuid uses
template<class Archive>
void serialize(Archive& ar, Uuid& uuid, const unsigned int /*version*/)
{
	ar & binary_data(&uuid, sizeof(uuid));
}
}// namespace ser20
