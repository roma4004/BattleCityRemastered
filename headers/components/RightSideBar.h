#pragma once
#include "geometry/Point.h"
#include "components/EventSystem.h"
#include <memory>
#include <vector>

struct ObjRectangle;
class GameConfig;
class EventSystem;
struct DrawUserInterfaceEvent;
struct RespawnCountChangedToEvent;

class RightSideBar
{
	Point _pos{};
	bool _isRightSideBarEnabled{false};
	bool _isEnemyIconEnabled{false};
	unsigned short _enemiesRespawnCount{};
	unsigned short _playerOneRespawnCount{};
	unsigned short _playerTwoRespawnCount{};
	unsigned short _stageNumber{1u};
	GameConfig& _gameConfig;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void Draw() const;
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;
	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);

public:
	RightSideBar(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~RightSideBar() = default;
};
