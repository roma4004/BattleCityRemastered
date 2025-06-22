#pragma once

#include "point.h"
#include "../BaseObjProperty.h"

enum Direction : char8_t;
enum GameMode : char8_t;
class EventSystem;

struct PawnProperty
{
	BaseObjProperty baseObjProperty{};
	std::vector<std::shared_ptr<BaseObj>>* allObjects{nullptr};
	std::shared_ptr<EventSystem> events{nullptr};
	UPoint windowSize{};
	GameMode gameMode{};
	int tier{1};
	Direction dir{};
	float speed{};
};
