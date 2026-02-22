#pragma once

#include "../BaseObj.h"
#include "interfaces/IDrawable.h"
#include <memory>

enum class ObstacleType : char8_t;
enum class GameMode : char8_t;
class EventSystem;

class Obstacle : public BaseObj, public IDrawable
{
	using buuid = boost::uuids::uuid;

	virtual void Subscribe();
	virtual void SubscribeAsClient();

	virtual void Unsubscribe() const;
	virtual void UnsubscribeAsClient() const;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	GameMode _gameMode{};
	ObstacleType _obstacleType{};

	void Draw() const override;
	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

public:
	Obstacle(ObjRectangle rect, unsigned int color, int health, std::string name,
			 const std::shared_ptr<EventSystem>& events, buuid uuid, GameMode gameMode, ObstacleType obstacleType);

	~Obstacle() override;
};
