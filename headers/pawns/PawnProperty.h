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
class IDrawable;

struct PawnProperty
{
	BaseObjProperty baseObjProperty{};
	std::vector<std::shared_ptr<BaseObj>>* allObjects{nullptr};
	std::shared_ptr<EventSystem> events{nullptr};
	std::shared_ptr<Window> window{nullptr};
	std::shared_ptr<IDrawable> textureManager{nullptr};
	GameMode gameMode;
	int tier{1};
	Direction dir{};
	float speed{0.f};
};
