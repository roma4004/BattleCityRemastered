#pragma once

#include "Obstacle.h"

struct ObjRectangle;
struct Window;
class EventSystem;

class SteelWall final : public Obstacle
{
public:
	SteelWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	          boost::uuids::uuid uuid, GameMode gameMode, std::shared_ptr<SDL_Texture> textureCollection,
	          std::shared_ptr<SDL_Renderer> renderer);

	~SteelWall() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
