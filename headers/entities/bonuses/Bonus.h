#pragma once

#include "../BaseObj.h"
#include "interfaces/IPickupableBonus.h"
#include "interfaces/ITickUpdatable.h"
#include <chrono>
#include <memory>

enum GameMode : char8_t;
enum BonusType : char8_t;
struct BaseObjProperty;
class EventSystem;

class Bonus : public BaseObj, public ITickUpdatable, public IPickupableBonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::chrono::system_clock::time_point _creationTime{};

protected:
	GameMode _gameMode{};
	BonusType _bonusType{};
	milliseconds _effectDuration{0};
	milliseconds _lifetime{0};

	std::shared_ptr<EventSystem> _events{nullptr};

	void TickUpdate(float deltaTime) override;

	void Draw(const BaseObj* obj) const override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void PickUpBonus(const std::string& author, const std::string& fraction) override;

public:
	Bonus(const ObjRectangle& rect, std::shared_ptr<EventSystem> events,
	      milliseconds duration, milliseconds lifeTime, int color, std::string name, buuid uuid, GameMode gameMode,
	      BonusType bonusType);

	~Bonus() override;

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;
};
