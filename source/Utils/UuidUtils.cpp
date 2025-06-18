#include "../../headers/utils/UuidUtils.h"

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

using buuid = boost::uuids::uuid;
buuid UuidUtils::GetRandomUuid()
{
	static boost::uuids::random_generator uuidGenerator;

	return uuidGenerator();
}

buuid UuidUtils::GetNilUuid()
{
	return boost::uuids::nil_uuid();
}

std::string UuidUtils::GetStringUuid(buuid uuid)
{
	return boost::uuids::to_string(uuid);
}
