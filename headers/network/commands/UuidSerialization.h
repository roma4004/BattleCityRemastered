#pragma once

#include <boost/serialization/binary_object.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>//NOTE: required for commands serialization

namespace boost
{
	namespace serialization
	{

		template<class Archive>
		void serialize(Archive& ar, boost::uuids::uuid& uuid, const unsigned int /*version*/)
		{
			ar & make_binary_object(&uuid, sizeof(uuid));
		}

	}// namespace serialization
}// namespace boost
