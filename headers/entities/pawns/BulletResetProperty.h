#pragma once

#include "../ObjRectangle.h"
#include "../BulletCalibre.h"
#include <boost/uuid/uuid.hpp>
#include <string>

enum class Direction : char8_t;

struct BulletResetProperty
{
	using buuid = boost::uuids::uuid;

	ObjRectangle rect{};
	Direction dir{};
	int health{};
	std::string author{};
	std::string fraction{};
	buuid uuid{};
	BulletCalibre calibre{};
};
