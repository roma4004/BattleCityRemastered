#pragma once

#include "point.h"
#include "../BaseObjProperty.h"

enum class Direction : char8_t;
enum class GameMode : char8_t;
class BaseObj;
class EventSystem;

struct PawnProperty
{
	BaseObjProperty baseObjProperty{};
	std::vector<std::shared_ptr<BaseObj>>* allObjects{nullptr};
	std::shared_ptr<EventSystem> events{nullptr};
	int tier{1};
	float speed{};
	UPoint windowSize{};
	Direction dir{};
	GameMode gameMode{};
};
