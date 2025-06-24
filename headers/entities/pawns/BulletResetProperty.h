#pragma once

#include "../ObjRectangle.h"
#include <string>
#include <boost/uuid/uuid.hpp>

enum Direction : char8_t;

struct BulletResetProperty
{
	using buuid = boost::uuids::uuid;

	ObjRectangle rect{};
	int damage{};
	Direction dir{};
	double aoeRadius{};
	int color{};
	int health{};
	float speed{};
	int tier{};
	std::string author{};
	std::string fraction{};
	buuid uuid{};
};
