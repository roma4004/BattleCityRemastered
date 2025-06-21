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
	double aoeRadius{};
	int color{};
	int health{};
	Direction dir{};
	float speed{};
	std::string author{};
	std::string fraction{};
	int tier{};
	buuid uuid{};
};
