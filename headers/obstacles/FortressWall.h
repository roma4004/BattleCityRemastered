#pragma once

#include "Obstacle.h"
#include "../BaseObj.h"
#include "../interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>
#include <variant>

struct Window;
class EventSystem;
class SteelWall;
class BrickWall;

class FortressWall final : public Obstacle, ITickUpdatable
{
	ObjRectangle _rect;
	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	std::variant<std::unique_ptr<BrickWall>,
	             std::unique_ptr<SteelWall>> _obstacle;
	bool _isActiveShovel{false};
	std::chrono::system_clock::time_point _activateTimeShovel;
	int _cooldownShovelMs{0};


	void Subscribe() override;
	void SubscribeAsClient();
	void SubscribeBonus();

	void Unsubscribe() const override;
	void UnsubscribeBonus() const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void Hide();

public:
	FortressWall(const ObjRectangle& rect, std::shared_ptr<Window> window, const std::shared_ptr<EventSystem>& events,
	             std::vector<std::shared_ptr<BaseObj>>* allObjects, int id);

	~FortressWall() override;

	[[nodiscard]] std::string GetName() const override;
	[[nodiscard]] int GetId() const override;

	void BonusShovelSwitch();

	void TakeDamage(int damage) override;

	[[nodiscard]] bool IsBrickWall() const;
	[[nodiscard]] bool IsSteelWall() const;

	int GetHealth() const override;

	void SetHealth(int health) override;

	[[nodiscard]] bool GetIsPassable() const override;

	void SetIsPassable(bool value) override;

	[[nodiscard]] bool GetIsDestructible() const override;

	void SetIsDestructible(bool value) override;

	[[nodiscard]] bool GetIsPenetrable() const override;

	void SetIsPenetrable(bool value) override;

	[[nodiscard]] ObjRectangle GetShape() const override;
	void SetShape(ObjRectangle shape) override;

	[[nodiscard]] bool GetIsAlive() const override;
	void SetIsAlive(bool isAlive) override;
};
