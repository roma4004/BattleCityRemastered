#pragma once

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

	void Draw() const override;

protected:
	GameMode _gameMode{};
	ObstacleType _obstacleType{};
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	Obstacle(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, const std::string& name,
	         std::shared_ptr<EventSystem> events, boost::uuids::uuid uuid, GameMode gameMode,
	         ObstacleType obstacleType);

	~Obstacle() override;
};
