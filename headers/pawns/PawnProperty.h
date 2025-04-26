#pragma once

#include "../BaseObjProperty.h"

#include <memory>
#include <vector>

enum Direction : char8_t;
enum GameMode : char8_t;
struct ObjRectangle;
struct Window;
struct UPoint;
class BaseObj;
class IMoveBeh;
class EventSystem;

struct PawnProperty
{
	BaseObjProperty baseObjProperty;
	std::shared_ptr<Window> window;
	Direction dir{};
	float speed{0.f};
	std::vector<std::shared_ptr<BaseObj>>* allObjects{nullptr};
	std::shared_ptr<EventSystem> events{nullptr};
	int tier{1};
	GameMode gameMode;
};
