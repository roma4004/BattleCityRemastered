#pragma once

#include "../BaseObj.h"
#include "../Point.h"
#include "../interfaces/ITickUpdatable.h"
#include "../obstacles/BrickWall.h"
#include "../obstacles/SteelWall.h"

#include <chrono>
#include <memory>
#include <variant>

class EventSystem;
// class SteelWall;
// class BrickWall;

class FortressWall final : public BaseObj, ITickUpdatable
{
	ObjRectangle _rect;
	UPoint _windowSize{.x = 0, .y = 0};
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::string _name;

	std::variant<std::unique_ptr<BrickWall>,
	             std::unique_ptr<SteelWall>> _obstacle;
	bool _isActiveShovel{false};
	std::chrono::system_clock::time_point _activateTimeShovel;
	int _cooldownShovelMs{0};
	int _obstacleId{0};

	void Subscribe();
	void Unsubscribe() const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void Hide();

public:
	FortressWall(const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
	             const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	             int obstacleId);
	//TODO: remove allObjects, move spawn check to separated spawner logic

	~FortressWall() override;

	[[nodiscard]] std::string GetName() const;
	[[nodiscard]] int GetId() const;

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
