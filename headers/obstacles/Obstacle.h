#pragma once

#include "../BaseObj.h"
#include "../GameMode.h"

#include <memory>

class EventSystem;
struct Window;

class Obstacle : public BaseObj
{
	std::shared_ptr<Window> _window{nullptr};

protected:
	GameMode _gameMode{Demo};
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	Obstacle(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, const std::string& name,
	         std::shared_ptr<EventSystem> events, int id, GameMode gameMode);

	~Obstacle() override;

	virtual void Subscribe();
	virtual void SubscribeAsClient();

	virtual void Unsubscribe() const;
	virtual void UnsubscribeAsClient() const;

	void Draw() const override;
};
