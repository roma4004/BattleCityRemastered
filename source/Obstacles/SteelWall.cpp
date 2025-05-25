#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

SteelWall::SteelWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const boost::uuids::uuid uuid, const GameMode gameMode,
                     std::shared_ptr<SDL_Texture> textureCollection,
                     std::shared_ptr<SDL_Renderer> renderer)
	: Obstacle{std::move(rect),
	           0xaaaaaa,
	           1,
	           std::move(window),
	           "SteelWall",
	           std::move(events),
	           uuid,
	           gameMode,
	           Steel,
	           std::move(textureCollection),
	           std::move(renderer)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);

	//TODO: remove this after load steel texture
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(); });
}

SteelWall::~SteelWall()
{
	//TODO: remove this after load steel texture
	_events->RemoveListener("Draw", _nameWithUuid);
}

void SteelWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("Statistics_SteelWallDied", author, fraction);
	}
}
