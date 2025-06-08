#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
struct Window;
class EventSystem;

class BrickWall final : public Obstacle
{
	using buuid = boost::uuids::uuid;

public:
	BrickWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events, buuid uuid,
	          GameMode gameMode, std::shared_ptr<IDrawable> textureManager);

	~BrickWall() override;
};
