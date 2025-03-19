#pragma once

#include "../BaseObj.h"
#include "../bonuses/BonusStatus.h"
#include "../interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>
#include <variant>

enum GameMode : char8_t;
struct Window;
class EventSystem;
class SteelWall;
class BrickWall;

class FortressWall final : public BaseObj, public ITickUpdatable
{
	ObjRectangle _rect;
	GameMode _gameMode{};
	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	std::variant<std::unique_ptr<BrickWall>,
	             std::unique_ptr<SteelWall>> _obstacle;

	BonusStatus _shovel{};

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();
	void SubscribeBonus();

	void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;
	void UnsubscribeBonus() const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void OnPlayerShovelCooldownEnd();

	void OnEnemyPickupShovel();

public:
	FortressWall(const ObjRectangle& rect, std::shared_ptr<Window> window, const std::shared_ptr<EventSystem>& events,
	             std::vector<std::shared_ptr<BaseObj>>* allObjects, int id, GameMode gameMode);

	~FortressWall() override;

	[[nodiscard]] std::string GetName() const override;
	[[nodiscard]] int GetId() const override;

	void OnPlayerPickupShovel();

	void TakeDamage(int damage) override;

	[[nodiscard]] bool IsBrickWall() const;
	[[nodiscard]] bool IsSteelWall() const;

	[[nodiscard]] int GetHealth() const override;

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
