#pragma once

#include "../BaseObj.h"
#include "interfaces/IDrawable.h"
#include "interfaces/IPickupableBonus.h"
#include "interfaces/ITickUpdatable.h"
#include "utils/Timer.h"

enum class GameMode : char8_t;
enum class BonusType : char8_t;
struct BaseObjProperty;
class EventSystem;

class Bonus : public BaseObj, public IDrawable, public ITickUpdatable, public IPickupableBonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	Timer _lifeTimeTimer{};
	GameMode _gameMode{};
	BonusType _bonusType{};

protected:
	std::shared_ptr<EventSystem> _events{nullptr};

	void TickUpdate(double deltaTime) override;
	void Draw() const override;

	virtual void EmitPickupEvent(const std::string& author, const std::string& fraction) = 0;

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
