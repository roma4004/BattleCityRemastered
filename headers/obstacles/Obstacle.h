#pragma once

#include "../BaseObj.h"

#include <memory>

class EventSystem;
struct Window;

class Obstacle : public BaseObj
{
	std::shared_ptr<Window> _window{nullptr};

public:
	Obstacle(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, const std::string& name,
	         std::shared_ptr<EventSystem> events, int id);

	~Obstacle() override;

	virtual void Subscribe();
	virtual void Unsubscribe() const;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	void Draw() const override;

	[[nodiscard]] virtual std::string GetName() const;
	[[nodiscard]] virtual int GetId() const;
};
