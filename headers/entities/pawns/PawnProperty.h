#pragma once

#include "../BaseObjProperty.h"
#include "enums/Author.h"
#include <memory>
#include <vector>

enum class Direction : char8_t;
class BaseObj;
class EventSystem;

struct PawnProperty
{
	BaseObjProperty baseObjProperty{};
	const std::vector<std::shared_ptr<BaseObj>>& allObjects;
	std::shared_ptr<EventSystem> events{nullptr};
	unsigned short tier{1u};
	double speed{};
	Direction dir{};
	Author author{};
};
