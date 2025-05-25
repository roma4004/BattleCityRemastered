#pragma once

#include "SDL.h"
#include "../BaseObj.h"
#include <memory>

enum ObstacleType : char8_t;
enum GameMode : char8_t;
class EventSystem;
struct Window;

class Obstacle : public BaseObj
{
	std::shared_ptr<Window> _window{nullptr};

	virtual void Subscribe();
	virtual void SubscribeAsClient();

	virtual void Unsubscribe() const;
	virtual void UnsubscribeAsClient() const;

	TextureManager _drawTexture;

protected:
	GameMode _gameMode{};
	ObstacleType _obstacleType{};
	std::shared_ptr<EventSystem> _events{nullptr};
	void Draw() const override;

public:
	Obstacle(ObjRectangle rect, int color, int health, std::shared_ptr<Window> window, std::string name,
	         std::shared_ptr<EventSystem> events, boost::uuids::uuid uuid, GameMode gameMode, ObstacleType obstacleType,
	         std::shared_ptr<SDL_Texture> texture, std::shared_ptr<SDL_Renderer> renderer);

	~Obstacle() override;
};
