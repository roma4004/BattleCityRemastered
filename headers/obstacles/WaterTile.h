#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
struct Window;
class EventSystem;

class WaterTile final : public Obstacle
{
public:
	WaterTile(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	          boost::uuids::uuid uuid, GameMode gameMode, std::shared_ptr<SDL_Texture> textureCollection,
	          std::shared_ptr<SDL_Renderer> renderer);

	~WaterTile() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
