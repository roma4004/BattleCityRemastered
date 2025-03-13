#pragma once

#include "../BaseObj.h"
#include "../EventSystem.h"
#include "../GameMode.h"
#include "../application/Window.h"
#include "../interfaces/IPickupableBonus.h"
#include "../interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>

class Bonus : public BaseObj, public ITickUpdatable, public IPickupableBonus
{
	std::shared_ptr<Window> _window{nullptr};

	std::chrono::system_clock::time_point _creationTime;

protected:
	int _id;
	GameMode _gameMode{Demo};
	std::chrono::milliseconds _bonusDuration{0};
	std::chrono::milliseconds _lifetime{0};

	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};

	void TickUpdate(float deltaTime) override;

	void Draw() const override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override = 0;

public:
	Bonus(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	      std::chrono::milliseconds duration, std::chrono::milliseconds lifeTime, int color, std::string name, int id,
	      GameMode gameMode);

	~Bonus() override;

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;

	void PickUpBonus(const std::string& author, const std::string& fraction) override = 0;

	int GetId() const;
};
