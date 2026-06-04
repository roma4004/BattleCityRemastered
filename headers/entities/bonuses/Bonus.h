#pragma once

#include "../BaseObj.h"
#include "interfaces/IDrawable.h"
#include "interfaces/IPickupableBonus.h"
#include "interfaces/ITickUpdatable.h"

enum class GameMode : char8_t;
enum class BonusType : char8_t;
struct BaseObjProperty;
class EventSystem;

class Bonus : public BaseObj, public IDrawable, public ITickUpdatable, public IPickupableBonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::chrono::system_clock::time_point _creationTime{};

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	milliseconds _lifetime{};
	GameMode _gameMode{};
	BonusType _bonusType{};

	void TickUpdate(double deltaTime) override;
	void Draw() const override;

public:
	Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
		  std::string name, buuid uuid, GameMode gameMode, BonusType bonusType);

	~Bonus() override;

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();

	void Unsubscribe() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
