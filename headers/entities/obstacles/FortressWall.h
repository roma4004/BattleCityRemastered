#pragma once

#include "BrickWall.h"
#include "SteelWall.h"
#include "../BaseObj.h"
#include "../bonuses/BonusStatus.h"
#include <chrono>
#include <memory>
#include <variant>

enum GameMode : char8_t;
class EventSystem;

class FortressWall final : public BaseObj
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	GameMode _gameMode{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{};

	std::variant<std::unique_ptr<BrickWall>,
	             std::unique_ptr<SteelWall>> _obstacle{};

	BonusStatus _shovel{};

	void Subscribe();
	void SubscribeAsClient();
	void SubscribeBonus();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;
	void UnsubscribeBonus() const;

	void Draw(const BaseObj* obj) const override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void OnPlayerShovelCooldownEnd();

	void OnEnemyPickupShovel();

public:
	FortressWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events,
	             std::vector<std::shared_ptr<BaseObj>>* allObjects, buuid uuid, GameMode gameMode);

	~FortressWall() override;

	void OnPlayerPickupShovel();
	//TODO: move to private section after rewrite unit test ShovelPickUpByEnemyThenFortressWallSteelWallHide

	[[nodiscard]] const std::string& GetName() const override;
	[[nodiscard]] buuid GetUuid() const override;

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

	[[nodiscard]] ObjRectangle GetRect() const override;
	void SetRect(ObjRectangle rect) override;

	[[nodiscard]] bool GetIsAlive() const override;
	void SetIsAlive(bool isAlive) override;
};
