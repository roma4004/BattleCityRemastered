#pragma once
#include "Point.h"
#include "enums/GameMode.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class RightSideBar
{
	Point _pos{};
	UPoint _windowSize{};
	bool _isRightSideBarEnabled{false};
	bool _isEnemyIconEnabled{false};
	unsigned short _enemiesRespawnCount{};
	unsigned short _playerOneRespawnCount{};
	unsigned short _playerTwoRespawnCount{};
	unsigned short _stageNumber{1};
	GameMode _gameMode{};

	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};

	void Subscribe();
	void Unsubscribe() const;
	void Draw() const;
	void OnRespawnCountChangedTo(const std::string& objectName, unsigned short respawnCount);

public:
	RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events);
	~RightSideBar();
};
