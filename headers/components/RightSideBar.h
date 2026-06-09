#pragma once
#include "Point.h"
#include "enums/GameMode.h"

struct ObjRectangle;
class EventSystem;

class RightSideBar
{
	Point _pos;
	UPoint _windowSize;
	bool _isRightSideBarEnabled{false};
	bool _isEnemyIconEnabled{false};
	int _enemiesRespawnCount{};
	int _playerOneRespawnCount{};
	int _playerTwoRespawnCount{};
	int _currentStageNumber{1};
	std::string _objectName{};
	GameMode _gameMode{};

	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};

	void Subscribe();
	void Unsubscribe();

public:
	RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events);
	~RightSideBar();
};
