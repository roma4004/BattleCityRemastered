#pragma once

#include "../BaseObj.h"
#include <memory>

enum ObstacleType : char8_t;
enum GameMode : char8_t;
class EventSystem;
class TextureManager;

class Obstacle : public BaseObj
{
	using buuid = boost::uuids::uuid;

	virtual void Subscribe();
	virtual void SubscribeAsClient();

	virtual void Unsubscribe() const;
	virtual void UnsubscribeAsClient() const;

protected:
	GameMode _gameMode{};
	ObstacleType _obstacleType{};
	std::shared_ptr<EventSystem> _events{nullptr};

	void Draw(const BaseObj* obj) const override;
	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

public:
	Obstacle(ObjRectangle rect, int color, int health, std::string name, std::shared_ptr<EventSystem> events,
	         buuid uuid, GameMode gameMode, ObstacleType obstacleType);

	~Obstacle() override;
};
