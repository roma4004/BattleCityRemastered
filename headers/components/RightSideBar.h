#pragma once
#include "Point.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include <memory>
#include <vector>

struct ObjRectangle;
class EventSystem;

class RightSideBar
{
	Point _pos{};
	bool _isRightSideBarEnabled{false};
	bool _isEnemyIconEnabled{false};
	unsigned short _enemiesRespawnCount{};
	unsigned short _playerOneRespawnCount{};
	unsigned short _playerTwoRespawnCount{};
	unsigned short _stageNumber{1u};
	GameMode _gameMode{};

	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void Draw() const;
	void OnRespawnCountChangedTo(const std::string& objectName, unsigned short respawnCount);

public:
	explicit RightSideBar(const std::shared_ptr<EventSystem>& events);

	~RightSideBar() = default;
};
