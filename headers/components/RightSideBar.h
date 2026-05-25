#pragma once
#include "Point.h"
class EventSystem;

class RightSideBar
{
	Point _pos;
	UPoint _windowSize;
	bool _isRightSideBarEnabled;
	bool _isEnemyIconEnabled;
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};

	void Subscribe();
	void Unsubscribe() const;

public:
	RightSideBar(UPoint windowSize, const std::shared_ptr<EventSystem>& events);
	~RightSideBar();
};
