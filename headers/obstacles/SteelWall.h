#pragma once

#include "Obstacle.h"

struct ObjRectangle;
struct Window;
class EventSystem;

class SteelWall final : public Obstacle
{
	using buuid = boost::uuids::uuid;

public:
	SteelWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events, buuid uuid,
	          GameMode gameMode, std::shared_ptr<IDrawable> textureManager);

	~SteelWall() override;
};
