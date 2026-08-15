#pragma once

#include <ser20/ser20.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>//NOTE: required for commands serialization

namespace ser20
{
using buuid = boost::uuids::uuid;

template<class Archive>
void serialize(Archive& ar, buuid& uuid, const unsigned int /*version*/)
{
	ar & binary_data(&uuid, sizeof(uuid));
}
}// namespace ser20
