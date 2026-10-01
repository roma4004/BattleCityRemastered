#pragma once
#include "components/EventSystem.h"
#include <memory>
#include <vector>

struct DrawUserInterfaceEvent;
struct RespawnCountChangedToEvent;
struct UiTable;
class GameConfig;

class RightSideBar final
{
	unsigned short _enemiesRespawnCount{};
	unsigned short _playerOneRespawnCount{};
	unsigned short _playerTwoRespawnCount{};
	unsigned short _stageNumber{1u};
	const GameConfig& _gameConfig;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void Draw() const;
	[[nodiscard]] UiTable EnemiesTable() const;
	[[nodiscard]] UiTable CountersTable() const;
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;
	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);

public:
	RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
